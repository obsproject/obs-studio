#include <windows.h>
#include "graphics-hook.h"

#define VK_USE_PLATFORM_WIN32_KHR

#include <malloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>

#undef VK_LAYER_EXPORT
#if defined(WIN32)
#define VK_LAYER_EXPORT __declspec(dllexport)
#else
#define VK_LAYER_EXPORT
#endif

#include <vulkan/vulkan_win32.h>

#define COBJMACROS
#include <dxgi.h>
#include <d3d11.h>

#include "vulkan-capture.h"

/* ======================================================================== */
/* defs/statics                                                                  */

/* shorten stuff because dear GOD is vulkan unclean. */
#define VKAPI VKAPI_CALL
#define VkFunc PFN_vkVoidFunction
#define EXPORT VK_LAYER_EXPORT

#define OBJ_MAX 16

/* use the loader's dispatch table pointer as a key for internal data maps */
#define GET_LDT(x) (*(void **)x)

/* clang-format off */
static const GUID dxgi_factory1_guid =
{0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87}};
static const GUID dxgi_resource_guid =
{0x035f3ab4, 0x482e, 0x4e50, {0xb4, 0x1f, 0x8a, 0x7f, 0x8b, 0xd8, 0x96, 0x0b}};
/* clang-format on */

static bool vulkan_seen = false;
static SRWLOCK mutex = SRWLOCK_INIT; // Faster CRITICAL_SECTION

/* ======================================================================== */
/* hook data                                                                */

struct vk_swap_data {
	VkSwapchainKHR sc;
	VkExtent2D image_extent;
	VkFormat format;
	HWND hwnd;
	VkImage export_image;
	bool layout_initialized;
	VkDeviceMemory export_mem;
	VkImage swap_images[OBJ_MAX];
	uint32_t image_count;

	HANDLE handle;
	struct shtex_data *shtex_info;
	ID3D11Texture2D *d3d11_tex;
	bool captured;
};

struct vk_queue_data {
	VkQueue queue;
	uint32_t fam_idx;
};

struct vk_cmd_pool_data {
	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_buffers[OBJ_MAX];
	VkFence fences[OBJ_MAX];
	bool cmd_buffer_busy[OBJ_MAX];
	uint32_t image_count;
};

struct vk_data {
	bool valid;

	struct vk_device_funcs funcs;
	VkPhysicalDevice phy_device;
	VkDevice device;
	struct vk_swap_data swaps[OBJ_MAX];
	struct vk_swap_data *cur_swap;
	uint32_t swap_idx;

	struct vk_queue_data queues[OBJ_MAX];
	uint32_t queue_count;

	struct vk_cmd_pool_data cmd_pools[OBJ_MAX];
	VkExternalMemoryProperties external_mem_props;

	struct vk_inst_data *inst_data;

	VkAllocationCallbacks ac_storage;
	const VkAllocationCallbacks *ac;

	ID3D11Device *d3d11_device;
	ID3D11DeviceContext *d3d11_context;
};

static struct vk_swap_data *get_swap_data(struct vk_data *data,
					  VkSwapchainKHR sc)
{
	for (int i = 0; i < OBJ_MAX; i++) {
		if (data->swaps[i].sc == sc) {
			return &data->swaps[i];
		}
	}

	debug("get_swap_data failed, swapchain not found");
	return NULL;
}

static struct vk_swap_data *get_new_swap_data(struct vk_data *data)
{
	for (int i = 0; i < OBJ_MAX; i++) {
		if (data->swaps[i].sc == VK_NULL_HANDLE) {
			return &data->swaps[i];
		}
	}

	debug("get_new_swap_data failed, no more free slot");
	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline size_t find_obj_idx(void *objs[], void *obj)
{
	size_t idx = SIZE_MAX;

	AcquireSRWLockExclusive(&mutex);
	for (size_t i = 0; i < OBJ_MAX; i++) {
		if (objs[i] == obj) {
			idx = i;
			break;
		}
	}
	ReleaseSRWLockExclusive(&mutex);

	return idx;
}

static size_t get_obj_idx(void *objs[], void *obj)
{
	size_t idx = SIZE_MAX;

	AcquireSRWLockExclusive(&mutex);
	for (size_t i = 0; i < OBJ_MAX; i++) {
		if (objs[i] == obj) {
			idx = i;
			break;
		}
		if (!objs[i] && idx == SIZE_MAX) {
			idx = i;
		}
	}
	ReleaseSRWLockExclusive(&mutex);
	return idx;
}

/* ------------------------------------------------------------------------- */

static struct vk_data device_data[OBJ_MAX] = {0};
static void *devices[OBJ_MAX] = {0};

static inline struct vk_data *get_device_data(void *dev)
{
	size_t idx = get_obj_idx(devices, GET_LDT(dev));
	if (idx == SIZE_MAX) {
		debug("out of device slots");
		return NULL;
	}

	return &device_data[idx];
}

static void vk_shtex_clear_fence(struct vk_data *data,
				 struct vk_cmd_pool_data *pool_data,
				 uint32_t image_idx)
{
	VkFence fence = pool_data->fences[image_idx];
	if (pool_data->cmd_buffer_busy[image_idx]) {
		VkDevice device = data->device;
		struct vk_device_funcs *funcs = &data->funcs;
		funcs->WaitForFences(device, 1, &fence, VK_TRUE, ~0ull);
		funcs->ResetFences(device, 1, &fence);
		pool_data->cmd_buffer_busy[image_idx] = false;
	}
}

static void vk_shtex_wait_until_pool_idle(struct vk_data *data,
					  struct vk_cmd_pool_data *pool_data)
{
	for (uint32_t image_idx = 0; image_idx < pool_data->image_count;
	     image_idx++) {
		vk_shtex_clear_fence(data, pool_data, image_idx);
	}
}

static void vk_shtex_wait_until_idle(struct vk_data *data)
{
	for (uint32_t fam_idx = 0; fam_idx < _countof(data->cmd_pools);
	     fam_idx++) {
		struct vk_cmd_pool_data *pool_data = &data->cmd_pools[fam_idx];
		if (pool_data->cmd_pool != VK_NULL_HANDLE)
			vk_shtex_wait_until_pool_idle(data, pool_data);
	}
}

static void vk_shtex_free(struct vk_data *data)
{
	capture_free();

	vk_shtex_wait_until_idle(data);

	for (int swap_idx = 0; swap_idx < OBJ_MAX; swap_idx++) {
		struct vk_swap_data *swap = &data->swaps[swap_idx];

		if (swap->export_image)
			data->funcs.DestroyImage(data->device,
						 swap->export_image, data->ac);

		if (swap->export_mem)
			data->funcs.FreeMemory(data->device, swap->export_mem,
					       NULL);

		if (swap->d3d11_tex) {
			ID3D11Resource_Release(swap->d3d11_tex);
		}

		swap->handle = INVALID_HANDLE_VALUE;
		swap->d3d11_tex = NULL;
		swap->export_mem = VK_NULL_HANDLE;
		swap->export_image = VK_NULL_HANDLE;

		swap->captured = false;
	}

	if (data->d3d11_context) {
		ID3D11DeviceContext_Release(data->d3d11_context);
		data->d3d11_context = NULL;
	}
	if (data->d3d11_device) {
		ID3D11Device_Release(data->d3d11_device);
		data->d3d11_device = NULL;
	}

	data->cur_swap = NULL;

	hlog("------------------ vulkan capture freed ------------------");
}

static void vk_remove_device(void *dev)
{
	size_t idx = find_obj_idx(devices, GET_LDT(dev));
	if (idx == SIZE_MAX) {
		return;
	}

	struct vk_data *data = &device_data[idx];

	memset(data, 0, sizeof(*data));

	AcquireSRWLockExclusive(&mutex);
	devices[idx] = NULL;
	ReleaseSRWLockExclusive(&mutex);
}

/* ------------------------------------------------------------------------- */

struct vk_surf_data {
	VkSurfaceKHR surf;
	HWND hwnd;
	struct vk_surf_data *next;
};

struct vk_inst_data {
	bool valid;

	struct vk_inst_funcs funcs;
	struct vk_surf_data *surfaces;
};

static void *object_malloc(const VkAllocationCallbacks *ac, size_t size,
			   size_t alignment)
{
	return ac ? ac->pfnAllocation(ac->pUserData, size, alignment,
				      VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)
		  : _aligned_malloc(size, alignment);
}

static void object_free(const VkAllocationCallbacks *ac, void *memory)
{
	if (ac)
		ac->pfnFree(ac->pUserData, memory);
	else
		_aligned_free(memory);
}

static void insert_surf_data(struct vk_inst_data *data, VkSurfaceKHR surf,
			     HWND hwnd, const VkAllocationCallbacks *ac)
{
	struct vk_surf_data *surf_data = object_malloc(
		ac, sizeof(struct vk_surf_data), _Alignof(struct vk_surf_data));
	if (surf_data) {
		surf_data->surf = surf;
		surf_data->hwnd = hwnd;

		AcquireSRWLockExclusive(&mutex);
		struct vk_surf_data *next = data->surfaces;
		surf_data->next = next;
		data->surfaces = surf_data;
		ReleaseSRWLockExclusive(&mutex);
	}
}

static HWND find_surf_hwnd(struct vk_inst_data *data, VkSurfaceKHR surf)
{
	HWND hwnd = NULL;

	AcquireSRWLockExclusive(&mutex);
	struct vk_surf_data *surf_data = data->surfaces;
	while (surf_data) {
		if (surf_data->surf == surf) {
			hwnd = surf_data->hwnd;
			break;
		}
		surf_data = surf_data->next;
	}
	ReleaseSRWLockExclusive(&mutex);

	return hwnd;
}

static void erase_surf_data(struct vk_inst_data *data, VkSurfaceKHR surf,
			    const VkAllocationCallbacks *ac)
{
	AcquireSRWLockExclusive(&mutex);
	struct vk_surf_data *current = data->surfaces;
	if (current->surf == surf) {
		data->surfaces = current->next;
	} else {
		struct vk_surf_data *previous;
		do {
			previous = current;
			current = current->next;
		} while (current && current->surf != surf);

		if (current)
			previous->next = current->next;
	}
	ReleaseSRWLockExclusive(&mutex);

	object_free(ac, current);
}

/* ------------------------------------------------------------------------- */

static struct vk_inst_data inst_data[OBJ_MAX] = {0};
static void *instances[OBJ_MAX] = {0};

static struct vk_inst_data *get_inst_data(void *inst)
{
	size_t idx = get_obj_idx(instances, GET_LDT(inst));
	if (idx == SIZE_MAX) {
		debug("out of instance slots");
		return NULL;
	}

	vulkan_seen = true;
	return &inst_data[idx];
}

static inline struct vk_inst_funcs *get_inst_funcs(void *inst)
{
	struct vk_inst_data *data = get_inst_data(inst);
	return &data->funcs;
}

static void remove_instance(void *inst)
{
	size_t idx = find_obj_idx(instances, inst);
	if (idx == SIZE_MAX) {
		return;
	}

	struct vk_inst_data *data = &inst_data[idx];
	memset(data, 0, sizeof(*data));

	AcquireSRWLockExclusive(&mutex);
	instances[idx] = NULL;
	ReleaseSRWLockExclusive(&mutex);
}

/* ======================================================================== */
/* capture                                                                  */

static inline bool vk_shtex_init_d3d11(struct vk_data *data)
{
	D3D_FEATURE_LEVEL level_used;
	IDXGIFactory1 *factory;
	IDXGIAdapter *adapter;
	HRESULT hr;

	HMODULE d3d11 = load_system_library("d3d11.dll");
	if (!d3d11) {
		flog("failed to load d3d11: %d", GetLastError());
		return false;
	}

	HMODULE dxgi = load_system_library("dxgi.dll");
	if (!dxgi) {
		flog("failed to load dxgi: %d", GetLastError());
		return false;
	}

	HRESULT(WINAPI * create_factory)
	(REFIID, void **) = (void *)GetProcAddress(dxgi, "CreateDXGIFactory1");
	if (!create_factory) {
		flog("failed to get CreateDXGIFactory1 address: %d",
		     GetLastError());
		return false;
	}

	PFN_D3D11_CREATE_DEVICE create =
		(void *)GetProcAddress(d3d11, "D3D11CreateDevice");
	if (!create) {
		flog("failed to get D3D11CreateDevice address: %d",
		     GetLastError());
		return false;
	}

	hr = create_factory(&dxgi_factory1_guid, (void **)&factory);
	if (FAILED(hr)) {
		flog_hr("failed to create factory", hr);
		return false;
	}

	hr = IDXGIFactory1_EnumAdapters1(factory, 0,
					 (IDXGIAdapter1 **)&adapter);
	IDXGIFactory1_Release(factory);

	if (FAILED(hr)) {
		flog_hr("failed to create adapter", hr);
		return false;
	}

	static const D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
	};

	hr = create(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, feature_levels,
		    sizeof(feature_levels) / sizeof(D3D_FEATURE_LEVEL),
		    D3D11_SDK_VERSION, &data->d3d11_device, &level_used,
		    &data->d3d11_context);
	IDXGIAdapter_Release(adapter);

	if (FAILED(hr)) {
		flog_hr("failed to create device", hr);
		return false;
	}

	return true;
}

static inline bool vk_shtex_init_d3d11_tex(struct vk_data *data,
					   struct vk_swap_data *swap)
{
	IDXGIResource *dxgi_res;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = swap->image_extent.width;
	desc.Height = swap->image_extent.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;

	flog("OBS requesting %s texture format.  capture dimensions: %dx%d",
	     vk_format_to_str(swap->format), (int)desc.Width, (int)desc.Height);

	desc.Format = vk_format_to_dxgi(swap->format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	hr = ID3D11Device_CreateTexture2D(data->d3d11_device, &desc, NULL,
					  &swap->d3d11_tex);
	if (FAILED(hr)) {
		flog_hr("failed to create texture", hr);
		return false;
	}

	hr = ID3D11Device_QueryInterface(swap->d3d11_tex, &dxgi_resource_guid,
					 (void **)&dxgi_res);
	if (FAILED(hr)) {
		flog_hr("failed to get IDXGIResource", hr);
		return false;
	}

	hr = IDXGIResource_GetSharedHandle(dxgi_res, &swap->handle);
	IDXGIResource_Release(dxgi_res);

	if (FAILED(hr)) {
		flog_hr("failed to get shared handle", hr);
		return false;
	}

	return true;
}

static inline bool vk_shtex_init_vulkan_tex(struct vk_data *data,
					    struct vk_swap_data *swap)
{
	struct vk_device_funcs *funcs = &data->funcs;
	VkExternalMemoryFeatureFlags f =
		data->external_mem_props.externalMemoryFeatures;

	/* -------------------------------------------------------- */
	/* create texture                                           */

	VkExternalMemoryImageCreateInfo emici;
	emici.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	emici.pNext = NULL;
	emici.handleTypes =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;

	VkImageCreateInfo ici;
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.pNext = &emici;
	ici.flags = 0;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = swap->format;
	ici.extent.width = swap->image_extent.width;
	ici.extent.height = swap->image_extent.height;
	ici.extent.depth = 1;
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		    VK_IMAGE_USAGE_SAMPLED_BIT;
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ici.queueFamilyIndexCount = 0;
	ici.pQueueFamilyIndices = 0;
	ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult res;
	res = funcs->CreateImage(data->device, &ici, data->ac,
				 &swap->export_image);
	if (VK_SUCCESS != res) {
		flog("failed to CreateImage: %s", result_to_str(res));
		swap->export_image = VK_NULL_HANDLE;
		return false;
	}

	swap->layout_initialized = false;

	/* -------------------------------------------------------- */
	/* get image memory requirements                            */

	VkMemoryRequirements mr;
	bool use_gimr2 = f & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;

	if (use_gimr2) {
		VkMemoryDedicatedRequirements mdr = {0};
		mdr.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
		mdr.pNext = NULL;

		VkMemoryRequirements2 mr2 = {0};
		mr2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		mr2.pNext = &mdr;

		VkImageMemoryRequirementsInfo2 imri2 = {0};
		imri2.sType =
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		imri2.pNext = NULL;
		imri2.image = swap->export_image;

		funcs->GetImageMemoryRequirements2(data->device, &imri2, &mr2);
		mr = mr2.memoryRequirements;
	} else {
		funcs->GetImageMemoryRequirements(data->device,
						  swap->export_image, &mr);
	}

	/* -------------------------------------------------------- */
	/* get memory type index                                    */

	struct vk_inst_funcs *ifuncs = get_inst_funcs(data->phy_device);

	VkPhysicalDeviceMemoryProperties pdmp;
	ifuncs->GetPhysicalDeviceMemoryProperties(data->phy_device, &pdmp);

	uint32_t mem_type_idx = 0;

	for (; mem_type_idx < pdmp.memoryTypeCount; mem_type_idx++) {
		if ((mr.memoryTypeBits & (1 << mem_type_idx)) &&
		    (pdmp.memoryTypes[mem_type_idx].propertyFlags &
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			break;
		}
	}

	if (mem_type_idx == pdmp.memoryTypeCount) {
		flog("failed to get memory type index");
		funcs->DestroyImage(data->device, swap->export_image, data->ac);
		swap->export_image = VK_NULL_HANDLE;
		return false;
	}

	/* -------------------------------------------------------- */
	/* allocate memory                                          */

	VkImportMemoryWin32HandleInfoKHR imw32hi;
	imw32hi.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	imw32hi.pNext = NULL;
	imw32hi.name = NULL;
	imw32hi.handleType =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;
	imw32hi.handle = swap->handle;

	VkMemoryAllocateInfo mai;
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.pNext = &imw32hi;
	mai.allocationSize = mr.size;
	mai.memoryTypeIndex = mem_type_idx;

	VkMemoryDedicatedAllocateInfo mdai;
	mdai.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	mdai.pNext = NULL;
	mdai.buffer = VK_NULL_HANDLE;

	if (data->external_mem_props.externalMemoryFeatures &
	    VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) {
		mdai.image = swap->export_image;
		imw32hi.pNext = &mdai;
	}

	res = funcs->AllocateMemory(data->device, &mai, NULL,
				    &swap->export_mem);
	if (VK_SUCCESS != res) {
		flog("failed to AllocateMemory: %s", result_to_str(res));
		funcs->DestroyImage(data->device, swap->export_image, data->ac);
		swap->export_image = VK_NULL_HANDLE;
		return false;
	}

	/* -------------------------------------------------------- */
	/* bind image memory                                        */

	bool use_bi2 = f & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;

	if (use_bi2) {
		VkBindImageMemoryInfo bimi = {0};
		bimi.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
		bimi.image = swap->export_image;
		bimi.memory = swap->export_mem;
		bimi.memoryOffset = 0;
		res = funcs->BindImageMemory2(data->device, 1, &bimi);
	} else {
		res = funcs->BindImageMemory(data->device, swap->export_image,
					     swap->export_mem, 0);
	}
	if (VK_SUCCESS != res) {
		flog("%s failed: %s",
		     use_bi2 ? "BindImageMemory2" : "BindImageMemory",
		     result_to_str(res));
		funcs->DestroyImage(data->device, swap->export_image, data->ac);
		swap->export_image = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

static bool vk_shtex_init(struct vk_data *data, HWND window,
			  struct vk_swap_data *swap)
{
	if (!vk_shtex_init_d3d11(data)) {
		return false;
	}
	if (!vk_shtex_init_d3d11_tex(data, swap)) {
		return false;
	}
	if (!vk_shtex_init_vulkan_tex(data, swap)) {
		return false;
	}

	data->cur_swap = swap;

	swap->captured = capture_init_shtex(
		&swap->shtex_info, window, swap->image_extent.width,
		swap->image_extent.height, swap->image_extent.width,
		swap->image_extent.height, (uint32_t)swap->format, false,
		(uintptr_t)swap->handle);

	if (swap->captured) {
		if (global_hook_info->force_shmem) {
			flog("shared memory capture currently "
			     "unsupported; ignoring");
		}

		hlog("vulkan shared texture capture successful");
		return true;
	}

	return false;
}

static void vk_shtex_create_cmd_pool_objects(struct vk_data *data,
					     uint32_t fam_idx,
					     uint32_t image_count)
{
	struct vk_cmd_pool_data *pool_data = &data->cmd_pools[fam_idx];

	VkCommandPoolCreateInfo cpci;
	cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpci.pNext = NULL;
	cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cpci.queueFamilyIndex = fam_idx;

	VkResult res = data->funcs.CreateCommandPool(
		data->device, &cpci, data->ac, &pool_data->cmd_pool);
	debug_res("CreateCommandPool", res);

	VkCommandBufferAllocateInfo cbai;
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.pNext = NULL;
	cbai.commandPool = pool_data->cmd_pool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbai.commandBufferCount = image_count;

	res = data->funcs.AllocateCommandBuffers(data->device, &cbai,
						 pool_data->cmd_buffers);
	debug_res("AllocateCommandBuffers", res);
	for (uint32_t image_index = 0; image_index < image_count;
	     image_index++) {
		/* Dispatch table something or other. Well-designed API. */
		VkCommandBuffer cmd_buffer =
			pool_data->cmd_buffers[image_index];
		*(void **)cmd_buffer = *(void **)(data->device);

		VkFence *fence = &pool_data->fences[image_index];
		VkFenceCreateInfo fci = {0};
		fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fci.pNext = NULL;
		fci.flags = 0;
		res = data->funcs.CreateFence(data->device, &fci, data->ac,
					      fence);
		debug_res("CreateFence", res);
	}

	pool_data->image_count = image_count;
}

static void vk_shtex_destroy_fence(struct vk_data *data, bool *cmd_buffer_busy,
				   VkFence *fence)
{
	VkDevice device = data->device;

	if (*cmd_buffer_busy) {
		data->funcs.WaitForFences(device, 1, fence, VK_TRUE, ~0ull);
		*cmd_buffer_busy = false;
	}

	data->funcs.DestroyFence(device, *fence, data->ac);
	*fence = VK_NULL_HANDLE;
}

static void
vk_shtex_destroy_cmd_pool_objects(struct vk_data *data,
				  struct vk_cmd_pool_data *pool_data)
{
	for (uint32_t image_idx = 0; image_idx < pool_data->image_count;
	     image_idx++) {
		bool *cmd_buffer_busy = &pool_data->cmd_buffer_busy[image_idx];
		VkFence *fence = &pool_data->fences[image_idx];
		vk_shtex_destroy_fence(data, cmd_buffer_busy, fence);
	}

	data->funcs.DestroyCommandPool(data->device, pool_data->cmd_pool,
				       data->ac);
	pool_data->cmd_pool = VK_NULL_HANDLE;
	pool_data->image_count = 0;
}

static void vk_shtex_capture(struct vk_data *data,
			     struct vk_device_funcs *funcs,
			     struct vk_swap_data *swap, uint32_t idx,
			     VkQueue queue, const VkPresentInfoKHR *info)
{
	VkResult res = VK_SUCCESS;

	VkCommandBufferBeginInfo begin_info;
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	VkImageMemoryBarrier mb[2];
	VkImageMemoryBarrier *src_mb = &mb[0];
	VkImageMemoryBarrier *dst_mb = &mb[1];

	/* ------------------------------------------------------ */
	/* do image copy                                          */

	const uint32_t image_index = info->pImageIndices[idx];
	VkImage cur_backbuffer = swap->swap_images[image_index];

	uint32_t fam_idx = 0;
	for (uint32_t i = 0; i < data->queue_count; i++) {
		if (data->queues[i].queue == queue)
			fam_idx = data->queues[i].fam_idx;
	}

	if (fam_idx >= _countof(data->cmd_pools))
		return;

	struct vk_cmd_pool_data *pool_data = &data->cmd_pools[fam_idx];
	VkCommandPool *pool = &pool_data->cmd_pool;
	const uint32_t image_count = swap->image_count;
	if (pool_data->image_count < image_count) {
		if (*pool != VK_NULL_HANDLE)
			vk_shtex_destroy_cmd_pool_objects(data, pool_data);
		vk_shtex_create_cmd_pool_objects(data, fam_idx, image_count);
	}

	vk_shtex_clear_fence(data, pool_data, image_index);

	VkCommandBuffer cmd_buffer = pool_data->cmd_buffers[image_index];
	res = funcs->BeginCommandBuffer(cmd_buffer, &begin_info);

#ifdef MORE_DEBUGGING
	debug_res("BeginCommandBuffer", res);
#endif

	/* ------------------------------------------------------ */
	/* transition shared texture if necessary                 */

	if (!swap->layout_initialized) {
		VkImageMemoryBarrier imb;
		imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imb.pNext = NULL;
		imb.srcAccessMask = 0;
		imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imb.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imb.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imb.image = swap->export_image;
		imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imb.subresourceRange.baseMipLevel = 0;
		imb.subresourceRange.levelCount = 1;
		imb.subresourceRange.baseArrayLayer = 0;
		imb.subresourceRange.layerCount = 1;

		funcs->CmdPipelineBarrier(cmd_buffer,
					  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					  VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
					  NULL, 0, NULL, 1, &imb);

		swap->layout_initialized = true;
	}

	/* ------------------------------------------------------ */
	/* transition cur_backbuffer to transfer source state     */

	src_mb->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	src_mb->pNext = NULL;
	src_mb->srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	src_mb->dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	src_mb->oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	src_mb->newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	src_mb->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	src_mb->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	src_mb->image = cur_backbuffer;
	src_mb->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	src_mb->subresourceRange.baseMipLevel = 0;
	src_mb->subresourceRange.levelCount = 1;
	src_mb->subresourceRange.baseArrayLayer = 0;
	src_mb->subresourceRange.layerCount = 1;

	/* ------------------------------------------------------ */
	/* transition exportedTexture to transfer dest state      */

	dst_mb->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dst_mb->pNext = NULL;
	dst_mb->srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dst_mb->dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dst_mb->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dst_mb->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dst_mb->srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	dst_mb->dstQueueFamilyIndex = fam_idx;
	dst_mb->image = swap->export_image;
	dst_mb->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dst_mb->subresourceRange.baseMipLevel = 0;
	dst_mb->subresourceRange.levelCount = 1;
	dst_mb->subresourceRange.baseArrayLayer = 0;
	dst_mb->subresourceRange.layerCount = 1;

	funcs->CmdPipelineBarrier(cmd_buffer,
				  VK_PIPELINE_STAGE_TRANSFER_BIT |
					  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				  VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
				  NULL, 2, mb);

	/* ------------------------------------------------------ */
	/* copy cur_backbuffer's content to our interop image     */

	VkImageCopy cpy;
	cpy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	cpy.srcSubresource.mipLevel = 0;
	cpy.srcSubresource.baseArrayLayer = 0;
	cpy.srcSubresource.layerCount = 1;
	cpy.srcOffset.x = 0;
	cpy.srcOffset.y = 0;
	cpy.srcOffset.z = 0;
	cpy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	cpy.dstSubresource.mipLevel = 0;
	cpy.dstSubresource.baseArrayLayer = 0;
	cpy.dstSubresource.layerCount = 1;
	cpy.dstOffset.x = 0;
	cpy.dstOffset.y = 0;
	cpy.dstOffset.z = 0;
	cpy.extent.width = swap->image_extent.width;
	cpy.extent.height = swap->image_extent.height;
	cpy.extent.depth = 1;
	funcs->CmdCopyImage(cmd_buffer, cur_backbuffer,
			    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			    swap->export_image,
			    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);

	/* ------------------------------------------------------ */
	/* Restore the swap chain image layout to what it was 
	 * before.  This may not be strictly needed, but it is
	 * generally good to restore things to their original
	 * state.  */

	src_mb->srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	src_mb->dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	src_mb->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	src_mb->newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	dst_mb->srcQueueFamilyIndex = fam_idx;
	dst_mb->dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;

	funcs->CmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				  VK_PIPELINE_STAGE_TRANSFER_BIT |
					  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				  0, 0, NULL, 0, NULL, 2, mb);

	funcs->EndCommandBuffer(cmd_buffer);

	/* ------------------------------------------------------ */

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = NULL;
	submit_info.pWaitDstStageMask = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = NULL;

	VkFence fence = pool_data->fences[image_index];
	res = funcs->QueueSubmit(queue, 1, &submit_info, fence);

#ifdef MORE_DEBUGGING
	debug_res("QueueSubmit", res);
#endif

	if (res == VK_SUCCESS)
		pool_data->cmd_buffer_busy[image_index] = true;
}

static inline bool valid_rect(struct vk_swap_data *swap)
{
	return !!swap->image_extent.width && !!swap->image_extent.height;
}

static void vk_capture(struct vk_data *data, VkQueue queue,
		       const VkPresentInfoKHR *info)
{
	struct vk_swap_data *swap = NULL;
	HWND window = NULL;
	uint32_t idx = 0;

#ifdef MORE_DEBUGGING
	debug("QueuePresentKHR called on "
	      "devicekey %p, swapchain count %d",
	      &data->funcs, info->swapchainCount);
#endif

	/* use first swap chain associated with a window */
	for (; idx < info->swapchainCount; idx++) {
		struct vk_swap_data *cur_swap =
			get_swap_data(data, info->pSwapchains[idx]);
		window = cur_swap->hwnd;
		if (!!window) {
			swap = cur_swap;
			break;
		}
	}

	if (!window) {
		return;
	}

	if (capture_should_stop()) {
		vk_shtex_free(data);
	}
	if (capture_should_init()) {
		if (valid_rect(swap) && !vk_shtex_init(data, window, swap)) {
			vk_shtex_free(data);
			data->valid = false;
			flog("vk_shtex_init failed");
		}
	}
	if (capture_ready()) {
		if (swap != data->cur_swap) {
			vk_shtex_free(data);
			return;
		}

		vk_shtex_capture(data, &data->funcs, swap, idx, queue, info);
	}
}

static VkResult VKAPI OBS_QueuePresentKHR(VkQueue queue,
					  const VkPresentInfoKHR *info)
{
	struct vk_data *data = get_device_data(queue);
	struct vk_device_funcs *funcs = &data->funcs;

	if (data->valid) {
		vk_capture(data, queue, info);
	}
	return funcs->QueuePresentKHR(queue, info);
}

/* ======================================================================== */
/* setup hooks                                                              */

static inline bool is_inst_link_info(VkLayerInstanceCreateInfo *lici)
{
	return lici->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
	       lici->function == VK_LAYER_LINK_INFO;
}

static VkResult VKAPI OBS_CreateInstance(const VkInstanceCreateInfo *cinfo,
					 const VkAllocationCallbacks *ac,
					 VkInstance *p_inst)
{
	VkInstanceCreateInfo info = *cinfo;
	bool funcs_not_found = false;

	/* -------------------------------------------------------- */
	/* step through chain until we get to the link info         */

	VkLayerInstanceCreateInfo *lici = (void *)info.pNext;
	while (lici && !is_inst_link_info(lici)) {
		lici = (VkLayerInstanceCreateInfo *)lici->pNext;
	}

	if (lici == NULL) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	PFN_vkGetInstanceProcAddr gpa =
		lici->u.pLayerInfo->pfnNextGetInstanceProcAddr;

	/* -------------------------------------------------------- */
	/* move chain on for next layer                             */

	lici->u.pLayerInfo = lici->u.pLayerInfo->pNext;

	/* -------------------------------------------------------- */
	/* (HACK) Set api version to 1.1 if set to 1.0              */
	/* We do this to get our extensions working properly        */

	VkApplicationInfo ai;
	if (info.pApplicationInfo) {
		ai = *info.pApplicationInfo;
		if (ai.apiVersion < VK_API_VERSION_1_1)
			ai.apiVersion = VK_API_VERSION_1_1;
	} else {
		ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ai.pNext = NULL;
		ai.pApplicationName = NULL;
		ai.applicationVersion = 0;
		ai.pEngineName = NULL;
		ai.engineVersion = 0;
		ai.apiVersion = VK_API_VERSION_1_1;
	}

	info.pApplicationInfo = &ai;

	/* -------------------------------------------------------- */
	/* create instance                                          */

	PFN_vkCreateInstance create = (void *)gpa(NULL, "vkCreateInstance");

	VkResult res = create(&info, ac, p_inst);
	VkInstance inst = *p_inst;

	/* -------------------------------------------------------- */
	/* fetch the functions we need                              */

	struct vk_inst_data *data = get_inst_data(inst);
	struct vk_inst_funcs *funcs = &data->funcs;

#define GETADDR(x)                                     \
	do {                                           \
		funcs->x = (void *)gpa(inst, "vk" #x); \
		if (!funcs->x) {                       \
			flog("could not get instance " \
			     "address for %s",         \
			     #x);                      \
			funcs_not_found = true;        \
		}                                      \
	} while (false)

	GETADDR(GetInstanceProcAddr);
	GETADDR(DestroyInstance);
	GETADDR(CreateWin32SurfaceKHR);
	GETADDR(DestroySurfaceKHR);
	GETADDR(GetPhysicalDeviceMemoryProperties);
	GETADDR(GetPhysicalDeviceImageFormatProperties2);
#undef GETADDR

	data->valid = !funcs_not_found;
	return res;
}

static VkResult VKAPI OBS_DestroyInstance(VkInstance instance,
					  const VkAllocationCallbacks *ac)
{
	struct vk_inst_funcs *funcs = get_inst_funcs(instance);
	funcs->DestroyInstance(instance, ac);
	remove_instance(instance);
	return VK_SUCCESS;
}

static bool
vk_shared_tex_supported(struct vk_inst_funcs *funcs,
			VkPhysicalDevice phy_device, VkFormat format,
			VkImageUsageFlags usage,
			VkExternalMemoryProperties *external_mem_props)
{
	VkPhysicalDeviceImageFormatInfo2 info;
	VkPhysicalDeviceExternalImageFormatInfo external_info;

	external_info.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
	external_info.pNext = NULL;
	external_info.handleType =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT;

	info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
	info.pNext = &external_info;
	info.format = format;
	info.type = VK_IMAGE_TYPE_2D;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.flags = 0;
	info.usage = usage;

	VkExternalImageFormatProperties external_props = {0};
	external_props.sType =
		VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
	external_props.pNext = NULL;

	VkImageFormatProperties2 props = {0};
	props.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
	props.pNext = &external_props;

	VkResult result = funcs->GetPhysicalDeviceImageFormatProperties2(
		phy_device, &info, &props);

	*external_mem_props = external_props.externalMemoryProperties;

	const VkExternalMemoryFeatureFlags features =
		external_mem_props->externalMemoryFeatures;

	return ((VK_SUCCESS == result) &&
		(features & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT));
}

static inline bool is_device_link_info(VkLayerDeviceCreateInfo *lici)
{
	return lici->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
	       lici->function == VK_LAYER_LINK_INFO;
}

static VkResult VKAPI OBS_CreateDevice(VkPhysicalDevice phy_device,
				       const VkDeviceCreateInfo *info,
				       const VkAllocationCallbacks *ac,
				       VkDevice *p_device)
{
	struct vk_inst_data *idata = get_inst_data(phy_device);
	struct vk_inst_funcs *ifuncs = &idata->funcs;
	struct vk_data *data = NULL;

	VkResult ret = VK_ERROR_INITIALIZATION_FAILED;

	VkLayerDeviceCreateInfo *ldci = (void *)info->pNext;

	/* -------------------------------------------------------- */
	/* step through chain until we get to the link info         */

	while (ldci && !is_device_link_info(ldci)) {
		ldci = (VkLayerDeviceCreateInfo *)ldci->pNext;
	}

	if (!ldci) {
		goto fail;
	}

	PFN_vkGetInstanceProcAddr gipa;
	PFN_vkGetDeviceProcAddr gdpa;

	gipa = ldci->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	gdpa = ldci->u.pLayerInfo->pfnNextGetDeviceProcAddr;

	/* -------------------------------------------------------- */
	/* move chain on for next layer                             */

	ldci->u.pLayerInfo = ldci->u.pLayerInfo->pNext;

	/* -------------------------------------------------------- */
	/* create device and initialize hook data                   */

	PFN_vkCreateDevice createFunc =
		(PFN_vkCreateDevice)gipa(VK_NULL_HANDLE, "vkCreateDevice");

	ret = createFunc(phy_device, info, ac, p_device);
	if (ret != VK_SUCCESS) {
		goto fail;
	}

	VkDevice device = *p_device;

	data = get_device_data(*p_device);
	struct vk_device_funcs *dfuncs = &data->funcs;

	data->valid = false; /* set true below if it doesn't go to fail */
	data->phy_device = phy_device;
	data->device = device;

	/* -------------------------------------------------------- */
	/* fetch the functions we need                              */

	bool funcs_not_found = false;

#define GETADDR(x)                                         \
	do {                                               \
		dfuncs->x = (void *)gdpa(device, "vk" #x); \
		if (!dfuncs->x) {                          \
			flog("could not get device "       \
			     "address for %s",             \
			     #x);                          \
			funcs_not_found = true;            \
		}                                          \
	} while (false)

#define GETADDR_OPTIONAL(x)                                \
	do {                                               \
		dfuncs->x = (void *)gdpa(device, "vk" #x); \
	} while (false)

	GETADDR(GetDeviceProcAddr);
	GETADDR(DestroyDevice);
	GETADDR(CreateSwapchainKHR);
	GETADDR(DestroySwapchainKHR);
	GETADDR(QueuePresentKHR);
	GETADDR(AllocateMemory);
	GETADDR(FreeMemory);
	GETADDR(BindImageMemory);
	GETADDR(BindImageMemory2);
	GETADDR(GetSwapchainImagesKHR);
	GETADDR(CreateImage);
	GETADDR(DestroyImage);
	GETADDR(GetImageMemoryRequirements);
	GETADDR(GetImageMemoryRequirements2);
	GETADDR(BeginCommandBuffer);
	GETADDR(EndCommandBuffer);
	GETADDR(CmdCopyImage);
	GETADDR(CmdPipelineBarrier);
	GETADDR(GetDeviceQueue);
	GETADDR(QueueSubmit);
	GETADDR(CreateCommandPool);
	GETADDR(DestroyCommandPool);
	GETADDR(AllocateCommandBuffers);
	GETADDR(CreateFence);
	GETADDR(DestroyFence);
	GETADDR(WaitForFences);
	GETADDR(ResetFences);
#undef GETADDR_OPTIONAL
#undef GETADDR

	if (funcs_not_found) {
		goto fail;
	}

	if (!idata->valid) {
		flog("instance not valid");
		goto fail;
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				  VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (!vk_shared_tex_supported(ifuncs, phy_device, format, usage,
				     &data->external_mem_props)) {
		flog("texture sharing is not supported");
		goto fail;
	}

	data->inst_data = idata;

	data->ac = NULL;
	if (ac) {
		data->ac_storage = *ac;
		data->ac = &data->ac_storage;
	}

	data->valid = true;

fail:
	return ret;
}

static void VKAPI OBS_DestroyDevice(VkDevice device,
				    const VkAllocationCallbacks *ac)
{
	struct vk_data *data = get_device_data(device);
	if (!data)
		return;

	if (data->valid) {
		for (uint32_t fam_idx = 0; fam_idx < _countof(data->cmd_pools);
		     fam_idx++) {
			struct vk_cmd_pool_data *pool_data =
				&data->cmd_pools[fam_idx];
			if (pool_data->cmd_pool != VK_NULL_HANDLE) {
				vk_shtex_destroy_cmd_pool_objects(data,
								  pool_data);
			}
		}
	}

	data->queue_count = 0;

	vk_remove_device(device);
	data->funcs.DestroyDevice(device, ac);
}

static VkResult VKAPI
OBS_CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *cinfo,
		       const VkAllocationCallbacks *ac, VkSwapchainKHR *p_sc)
{
	struct vk_data *data = get_device_data(device);

	VkSwapchainCreateInfoKHR info = *cinfo;
	if (data->valid)
		info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	struct vk_device_funcs *funcs = &data->funcs;
	VkResult res = funcs->CreateSwapchainKHR(device, &info, ac, p_sc);
	debug_res("CreateSwapchainKHR", res);
	if ((res != VK_SUCCESS) || !data->valid)
		return res;

	VkSwapchainKHR sc = *p_sc;

	uint32_t count = 0;
	res = funcs->GetSwapchainImagesKHR(data->device, sc, &count, NULL);
	debug_res("GetSwapchainImagesKHR", res);

	struct vk_swap_data *swap = get_new_swap_data(data);
	if (count > 0) {
		if (count > OBJ_MAX)
			count = OBJ_MAX;

		res = funcs->GetSwapchainImagesKHR(data->device, sc, &count,
						   swap->swap_images);
		debug_res("GetSwapchainImagesKHR", res);
	}

	swap->sc = sc;
	swap->image_extent = cinfo->imageExtent;
	swap->format = cinfo->imageFormat;
	swap->hwnd = find_surf_hwnd(data->inst_data, cinfo->surface);
	swap->image_count = count;

	return VK_SUCCESS;
}

static void VKAPI OBS_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR sc,
					  const VkAllocationCallbacks *ac)
{
	struct vk_data *data = get_device_data(device);
	struct vk_device_funcs *funcs = &data->funcs;

	if (data->valid) {
		struct vk_swap_data *swap = get_swap_data(data, sc);
		if (swap) {
			if (data->cur_swap == swap) {
				vk_shtex_free(data);
			}

			swap->sc = VK_NULL_HANDLE;
			swap->hwnd = NULL;
		}
	}

	funcs->DestroySwapchainKHR(device, sc, ac);
}

static void VKAPI OBS_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex,
				     uint32_t queueIndex, VkQueue *pQueue)
{
	struct vk_data *data = get_device_data(device);
	struct vk_device_funcs *funcs = &data->funcs;

	funcs->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

	for (uint32_t i = 0; i < data->queue_count; ++i) {
		if (data->queues[i].queue == *pQueue)
			return;
	}

	if (data->queue_count < _countof(data->queues)) {
		data->queues[data->queue_count].queue = *pQueue;
		data->queues[data->queue_count].fam_idx = queueFamilyIndex;
		++data->queue_count;
	}
}

static VkResult VKAPI OBS_CreateWin32SurfaceKHR(
	VkInstance inst, const VkWin32SurfaceCreateInfoKHR *info,
	const VkAllocationCallbacks *ac, VkSurfaceKHR *surf)
{
	struct vk_inst_data *data = get_inst_data(inst);
	struct vk_inst_funcs *funcs = &data->funcs;

	VkResult res = funcs->CreateWin32SurfaceKHR(inst, info, ac, surf);
	if (res == VK_SUCCESS)
		insert_surf_data(data, *surf, info->hwnd, ac);
	return res;
}

static void VKAPI OBS_DestroySurfaceKHR(VkInstance inst, VkSurfaceKHR surf,
					const VkAllocationCallbacks *ac)
{
	struct vk_inst_data *data = get_inst_data(inst);
	struct vk_inst_funcs *funcs = &data->funcs;

	erase_surf_data(data, surf, ac);
	funcs->DestroySurfaceKHR(inst, surf, ac);
}

#define GETPROCADDR(func)              \
	if (!strcmp(name, "vk" #func)) \
		return (VkFunc)&OBS_##func;

static VkFunc VKAPI OBS_GetDeviceProcAddr(VkDevice dev, const char *name)
{
	struct vk_data *data = get_device_data(dev);
	struct vk_device_funcs *funcs = &data->funcs;

	debug_procaddr("vkGetDeviceProcAddr(%p, \"%s\")", dev, name);

	GETPROCADDR(GetDeviceProcAddr);
	GETPROCADDR(CreateDevice);
	GETPROCADDR(DestroyDevice);
	GETPROCADDR(CreateSwapchainKHR);
	GETPROCADDR(DestroySwapchainKHR);
	GETPROCADDR(QueuePresentKHR);
	GETPROCADDR(GetDeviceQueue);

	if (funcs->GetDeviceProcAddr == NULL)
		return NULL;
	return funcs->GetDeviceProcAddr(dev, name);
}

static VkFunc VKAPI OBS_GetInstanceProcAddr(VkInstance inst, const char *name)
{
	debug_procaddr("vkGetInstanceProcAddr(%p, \"%s\")", inst, name);

	/* instance chain functions we intercept */
	GETPROCADDR(GetInstanceProcAddr);
	GETPROCADDR(CreateInstance);
	GETPROCADDR(DestroyInstance);
	GETPROCADDR(CreateWin32SurfaceKHR);
	GETPROCADDR(DestroySurfaceKHR);

	/* device chain functions we intercept */
	GETPROCADDR(GetDeviceProcAddr);
	GETPROCADDR(CreateDevice);
	GETPROCADDR(DestroyDevice);

	struct vk_inst_funcs *funcs = get_inst_funcs(inst);
	if (funcs->GetInstanceProcAddr == NULL)
		return NULL;
	return funcs->GetInstanceProcAddr(inst, name);
}

#undef GETPROCADDR

EXPORT VkResult VKAPI OBS_Negotiate(VkNegotiateLayerInterface *nli)
{
	if (nli->loaderLayerInterfaceVersion >= 2) {
		nli->sType = LAYER_NEGOTIATE_INTERFACE_STRUCT;
		nli->pNext = NULL;
		nli->pfnGetInstanceProcAddr = OBS_GetInstanceProcAddr;
		nli->pfnGetDeviceProcAddr = OBS_GetDeviceProcAddr;
		nli->pfnGetPhysicalDeviceProcAddr = NULL;
	}

	const uint32_t cur_ver = CURRENT_LOADER_LAYER_INTERFACE_VERSION;

	if (nli->loaderLayerInterfaceVersion > cur_ver) {
		nli->loaderLayerInterfaceVersion = cur_ver;
	}

	return VK_SUCCESS;
}

bool hook_vulkan(void)
{
	static bool hooked = false;
	if (!hooked && vulkan_seen) {
		hlog("Hooked Vulkan");
		hooked = true;
	}
	return hooked;
}
