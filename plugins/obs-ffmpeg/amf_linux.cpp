#include <util/platform.h>
#include <util/util.hpp>
#include <util/pipe.h>
#include <util/dstr.h>
#include <obs.h>
#include <dlfcn.h>
#include <memory>
#include <vector>
#include <stdarg.h>
#include <unistd.h>
#include <unordered_set>
#include <string>

#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define AMFOBS_ENCODER "amf_encoder"
#define AMFOBS_ENCODER_NAME "amf_vulkan_impl"

#include "amf_gpu.hpp"

using namespace amf;
//-----------------------------------------------------------------------------------------------

#define AMFOBS_RETURN_IF_EGL_FAILED(_ret, format, ...) \
	AMFOBS_RETURN_IF_FALSE((eglGetError() == EGL_SUCCESS), _ret, format, ##__VA_ARGS__)

#define GET_DLL_ENTRYPOINT(h, w) w = reinterpret_cast<PFN_##w>(dlsym(h, #w)); if(w==nullptr) \
	{ error("Failed to aquire entrypoint %s", #w); return AMF_FAIL; };

#define GET_INSTANCE_ENTRYPOINT(i, w) w = reinterpret_cast<PFN_##w>(vkGetInstanceProcAddr(i, #w)); if(w==nullptr) \
    { error("Failed to aquire instance entrypoint %s", #w); return AMF_FAIL; };

//-----------------------------------------------------------------------------------------------
class amf_vulkan_impl : public amf_gpu, public amf::AMFSurfaceObserver {
public:
	amf_vulkan_impl() {}
	~amf_vulkan_impl() { terminate(); }

	// amf_gpu interface
	AMF_RESULT init(amf::AMFContext *context, uint32_t adapter) override;
	AMF_RESULT terminate() override;
	AMF_RESULT copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
				   amf::AMF_SURFACE_FORMAT format, int width, int height,
				   amf::AMFSurface **surface) override;
	AMF_RESULT get_luid(uint64_t *luid) override;
	AMF_RESULT get_vendor(uint32_t *vendor) override;

private:
	// AMFSurfaceObserver interface
	virtual void AMF_STD_CALL OnSurfaceDataRelease(amf::AMFSurface *pSurface);

	// helpers
	AMF_RESULT create_surface_from_handle(int fd, amf::AMF_SURFACE_FORMAT format, amf::AMF_PLANE_TYPE planeType,
					      int width, int height, amf::AMFSurface **surface);
	AMF_RESULT copy_to_surface(amf::AMFSurface *y, amf::AMFSurface *uv, amf::AMFSurface *out);
	AMF_RESULT init_egl();
	AMF_RESULT init_vulkan_table();
	AMF_RESULT init_vulkan_device(AMFContext *context, uint32_t adapter);

	amf::AMFContext2Ptr amf_context;
	amf::AMFComputePtr amf_compute;
	amf_handle vulkan_so = 0;
	amf_handle opengl_so = 0;

	// Vulkan functions:
	PFN_vkAllocateMemory vkAllocateMemory = nullptr;
	PFN_vkFreeMemory vkFreeMemory = nullptr;
	PFN_vkBindImageMemory vkBindImageMemory = nullptr;
	PFN_vkCreateImage vkCreateImage = nullptr;
	PFN_vkDestroyImage vkDestroyImage = nullptr;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
	PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
	PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;

	PFN_vkCreateInstance vkCreateInstance = nullptr;
	PFN_vkDestroyInstance vkDestroyInstance = nullptr;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
	PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
	PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = nullptr;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
	PFN_vkCreateDevice vkCreateDevice = nullptr;
	PFN_vkDestroyDevice vkDestroyDevice = nullptr;

	// EGL & OGL functions

	PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA = nullptr;
	PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA = nullptr;

	AMFVulkanDevice amfvk_device{};

	class egl_image // self destructuctor
	{
	public:
		egl_image(amf_vulkan_impl *host) : amf_host(host) {}
		~egl_image()
		{
			if (fd != 0)
				close(fd);
			if (egl_img != 0)
				eglDestroyImage(eglGetCurrentDisplay(), egl_img);
		}
		AMF_RESULT import_to_amf(GLuint tex, AMF_SURFACE_FORMAT format, AMF_PLANE_TYPE planeType, int width,
					 int height, AMFSurface **out);

	protected:
		amf_vulkan_impl *amf_host = nullptr;
		EGLImage egl_img = 0;
		int fd = 0;
	};
};
//-----------------------------------------------------------------------------------------------
amf_gpu *amf_gpu::create()
{
	return new amf_vulkan_impl();
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init(AMFContext *context, uint32_t adapter)
{
	AMF_RESULT res;

	res = init_vulkan_table();
	AMFOBS_RETURN_IF_FAILED(res, "init: init_vulkan_table failed with res = %d", res);

	res = init_vulkan_device(context, adapter);
	AMFOBS_RETURN_IF_FAILED(res, "init: init_vulkan_device failed with res = %d", res);

	res = AMFContext2Ptr(context)->InitVulkan(&amfvk_device);
	if (res != AMF_OK)
		res = AMF_NOT_SUPPORTED;
	AMFOBS_RETURN_IF_FAILED(res, "init: AMFContext2::InitVulkan() failed with res = %d", res);

	res = context->GetCompute(AMF_MEMORY_VULKAN, &amf_compute);
	AMFOBS_RETURN_IF_FAILED(res, "init: AMFContext2::GetCompute(AMF_MEMORY_VULKAN) failed with res = %d", res);

	res = init_egl();
	AMFOBS_RETURN_IF_FAILED(res, "init: init_egl() failed with res = %d", res);

	amf_context = AMFContext2Ptr(context);
	return AMF_OK;
} //-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init_vulkan_table()
{
	vulkan_so = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_GLOBAL);
	AMFOBS_RETURN_IF_FALSE(vulkan_so != 0, AMF_FAIL, "dlopen(libvulkan.so.1) failed");

	GET_DLL_ENTRYPOINT(vulkan_so, vkFreeMemory);
	GET_DLL_ENTRYPOINT(vulkan_so, vkAllocateMemory);
	GET_DLL_ENTRYPOINT(vulkan_so, vkFreeMemory);
	GET_DLL_ENTRYPOINT(vulkan_so, vkBindImageMemory);
	GET_DLL_ENTRYPOINT(vulkan_so, vkCreateImage);
	GET_DLL_ENTRYPOINT(vulkan_so, vkDestroyImage);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetImageMemoryRequirements);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetPhysicalDeviceMemoryProperties);
	GET_DLL_ENTRYPOINT(vulkan_so, vkCreateSemaphore);
	GET_DLL_ENTRYPOINT(vulkan_so, vkDestroySemaphore);
	GET_DLL_ENTRYPOINT(vulkan_so, vkCreateInstance);
	GET_DLL_ENTRYPOINT(vulkan_so, vkDestroyInstance);
	GET_DLL_ENTRYPOINT(vulkan_so, vkEnumeratePhysicalDevices);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetPhysicalDeviceFeatures);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetInstanceProcAddr);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetDeviceProcAddr);
	GET_DLL_ENTRYPOINT(vulkan_so, vkGetPhysicalDeviceQueueFamilyProperties2);
	GET_DLL_ENTRYPOINT(vulkan_so, vkEnumerateDeviceExtensionProperties);
	GET_DLL_ENTRYPOINT(vulkan_so, vkCreateDevice);
	GET_DLL_ENTRYPOINT(vulkan_so, vkDestroyDevice);
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init_egl()
{
	eglExportDMABUFImageQueryMESA =
		(PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)eglGetProcAddress("eglExportDMABUFImageQueryMESA");
	eglExportDMABUFImageMESA = (PFNEGLEXPORTDMABUFIMAGEMESAPROC)eglGetProcAddress("eglExportDMABUFImageMESA");

	AMFOBS_RETURN_IF_FALSE(eglExportDMABUFImageMESA != nullptr, AMF_NOT_SUPPORTED,
			       "init_egl: egl doesn't support eglExportDMABUFImageMESA");
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::terminate()
{
	amf_compute = nullptr;
	amf_context = nullptr;

	if (amfvk_device.hDevice != VK_NULL_HANDLE) {
		vkDestroyDevice(amfvk_device.hDevice, nullptr);
	}
	if (amfvk_device.hInstance != VK_NULL_HANDLE) {
		vkDestroyInstance(amfvk_device.hInstance, nullptr);
	}
	amfvk_device = {};

	if (vulkan_so) {
		dlclose(vulkan_so);
		vulkan_so = 0;
	}
	if (opengl_so) {
		dlclose(opengl_so);
		opengl_so = 0;
	}
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::create_surface_from_handle(int fd, AMF_SURFACE_FORMAT format, AMF_PLANE_TYPE planeType,
						       int width, int height, AMFSurface **surface)
{
	AMFOBS_RETURN_IF_FALSE(amf_context != nullptr, AMF_NOT_INITIALIZED, "amf_vulkan_impl not initialized");

	VkFormat formatVK = VK_FORMAT_UNDEFINED;
	switch (format) {
	case AMF_SURFACE_NV12:
		switch (planeType) {
		case AMF_PLANE_Y:
			formatVK = VK_FORMAT_R8_UNORM;
			break;
		case AMF_PLANE_UV:
			formatVK = VK_FORMAT_R8G8_UNORM;
			break;
		default:
			return AMF_INVALID_ARG;
		}

		break;
	case AMF_SURFACE_P010:
		switch (planeType) {
		case AMF_PLANE_Y:
			formatVK = VK_FORMAT_R16_SFLOAT; //AMF need to add support for VK_FORMAT_R16_UNORM
			break;
		case AMF_PLANE_UV:
			formatVK = VK_FORMAT_R16G16_SFLOAT;
			break;
		default:
			return AMF_INVALID_ARG;
		}
		break;
	default:
		return AMF_INVALID_FORMAT;
	}

	AMFVulkanDevice *vulkan_dev = (AMFVulkanDevice *)amf_compute->GetNativeContext();

	// config VkImageCreateInfo
	std::unique_ptr<amf_uint8[]> surface_data(
		new amf_uint8[sizeof(AMFVulkanSurface) + sizeof(AMFVulkanSurface1)]());

	AMFVulkanSurface *surfaceVK = (AMFVulkanSurface *)surface_data.get();
	AMFVulkanSurface1 *surfaceVK1 = (AMFVulkanSurface1 *)(surface_data.get() + sizeof(AMFVulkanSurface));

	VkResult vkres = VK_SUCCESS;

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.extent.depth = 1;

	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT; //0;

	imageCreateInfo.format = formatVK;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.mipLevels = 1;

	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.initialLayout =
		VK_IMAGE_LAYOUT_UNDEFINED; // changed to satisy validation layer VK_IMAGE_LAYOUT_PREINITIALIZED;

	VkImageFormatListCreateInfo formatList = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
	formatList.viewFormatCount = 1;
	std::vector<VkFormat> formats;
	formats.push_back(imageCreateInfo.format);
	formatList.pViewFormats = formats.data();

	VkExternalMemoryImageCreateInfo vkExternalMemoryImageCreateInfo = {};
	vkExternalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	vkExternalMemoryImageCreateInfo.handleTypes =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT; //VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

	//	VkImageDrmFormatModifierListCreateInfoEXT drmCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT};
	//	drmCreateInfo.drmFormatModifierCount = 1;
	//	drmCreateInfo.pDrmFormatModifiers = &drmMode;

	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_LINEAR
	// create creation chain
	imageCreateInfo.pNext = &formatList;
	formatList.pNext = &vkExternalMemoryImageCreateInfo;
	vkExternalMemoryImageCreateInfo.pNext = nullptr;

	// create vkimage
	vkres = vkCreateImage(vulkan_dev->hDevice, &imageCreateInfo, nullptr /* pAllocator */, &surfaceVK->hImage);
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "create_surface_from_handle: CreateSurface() vkCreateImage() failed, err = %d", vkres);

	VkMemoryRequirements memoryRequirements = {};
	VkPhysicalDeviceMemoryProperties memoryProps;
	bool needsMappableMemory = false;
	bool needsCoherentMemory = false;
	bool needLocalMemory = false;

	vkGetImageMemoryRequirements(vulkan_dev->hDevice, surfaceVK->hImage, &memoryRequirements);
	vkGetPhysicalDeviceMemoryProperties(vulkan_dev->hPhysicalDevice, &memoryProps);

	uint32_t memoryTypeIndex = UINT32_MAX;

	for (uint32_t nMemoryType = 0; nMemoryType < memoryProps.memoryTypeCount; ++nMemoryType) {
		if ((!needsCoherentMemory ||
		     (memoryProps.memoryTypes[nMemoryType].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) &&
		    (!needsMappableMemory ||
		     (memoryProps.memoryTypes[nMemoryType].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
			if ((memoryRequirements.memoryTypeBits & (1 << nMemoryType)) != 0) {
				memoryTypeIndex = nMemoryType;
				if (!needLocalMemory || (memoryProps.memoryTypes[nMemoryType].propertyFlags &
							 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
					break;
			}
		}
	}
	AMFOBS_RETURN_IF_FALSE(memoryTypeIndex != UINT32_MAX, AMF_FAIL,
			       "create_surface_from_handle: AllocateMemory() No memory request is found");

	// allocate vkimage mem
	VkMemoryAllocateInfo allocInfo = {};
	VkImportMemoryFdInfoKHR importFDinfo = {};
	VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo = {}; //This is needed for good interop from Vulkan to PAL

	dedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	dedicatedAllocateInfo.image = surfaceVK->hImage;

	importFDinfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
	importFDinfo.fd = fd;
	//import_fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;//VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT//TODO
	importFDinfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT; //TODO
	importFDinfo.pNext = &dedicatedAllocateInfo;

	allocInfo.pNext = &importFDinfo;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	allocInfo.memoryTypeIndex = memoryTypeIndex; //TODO
	vkres = vkAllocateMemory(vulkan_dev->hDevice, &allocInfo, nullptr, &surfaceVK->hMemory);
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "create_surface_from_handle: AllocateMemory() failed to vkAllocateMemory, Error = %d",
			       (int)vkres);
	AMFOBS_RETURN_IF_FALSE(surfaceVK->hMemory != VK_NULL_HANDLE, AMF_FAIL,
			       "create_surface_from_handle: AllocateMemory() cannot allocate memory");

	vkres = vkBindImageMemory(vulkan_dev->hDevice, surfaceVK->hImage, surfaceVK->hMemory, 0);

	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "create_surface_from_handle: vkBindImageMemory() failed, err = %d", vkres);

	surfaceVK->cbSizeof = sizeof(*surfaceVK);    // sizeof(AMFVulkanSurface)
	surfaceVK->pNext = surfaceVK1;               // reserved for extensions
						     // surface properties
	surfaceVK->iSize = memoryRequirements.size;  // memory size
	surfaceVK->eFormat = imageCreateInfo.format; // VkFormat
	surfaceVK->iWidth = imageCreateInfo.extent.width;
	surfaceVK->iHeight = imageCreateInfo.extent.height;

	surfaceVK->eCurrentLayout = imageCreateInfo.initialLayout;                              // VkImageLayout
	surfaceVK->eUsage = AMF_SURFACE_USAGE_SHADER_RESOURCE | AMF_SURFACE_USAGE_TRANSFER_SRC; // AMF_SURFACE_USAGE
	surfaceVK->eUsage |= AMF_SURFACE_USAGE_NO_TRANSITION;
	surfaceVK->eAccess = AMF_MEMORY_CPU_DEFAULT; // AMF_MEMORY_CPU_ACCESS

	surfaceVK1->cbSizeof = sizeof(*surfaceVK1);
	surfaceVK1->pNext = nullptr;
	surfaceVK1->eTiling = imageCreateInfo.tiling;

	VkExportSemaphoreCreateInfo exportInfo = {};
	exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
	exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.pNext = &exportInfo;
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	surfaceVK->Sync.bSubmitted = false;
	vkres = vkCreateSemaphore(vulkan_dev->hDevice, &semaphoreInfo, nullptr, &surfaceVK->Sync.hSemaphore);
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "create_surface_from_handle: vkCreateSemaphore() failed with %d", (int)vkres);

	AMF_RESULT res = amf_context->CreateSurfaceFromVulkanNative(surfaceVK, surface, this);
	AMFOBS_RETURN_IF_FAILED(res, "create_surface_from_handle: CreateSurfaceFromVulkanNative() failed");

	// success
	surface_data.release(); // will be deleted in callback

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
void AMF_STD_CALL amf_vulkan_impl::OnSurfaceDataRelease(AMFSurface *pSurface)
{
	if (pSurface == nullptr)
		return;
	pSurface->RemoveObserver(this);

	if (amf_compute != nullptr) {
		AMFVulkanDevice *vulkan_dev = (AMFVulkanDevice *)amf_compute->GetNativeContext();
		AMFVulkanView *view = (AMFVulkanView *)pSurface->GetPlaneAt(0)->GetNative();
		std::unique_ptr<amf_uint8[]> surface_data((amf_uint8 *)view->pSurface); // auto-delete

		AMFVulkanSurface *surfaceVK = (AMFVulkanSurface *)surface_data.get();

		if (surfaceVK->hImage != VK_NULL_HANDLE)
			vkDestroyImage(vulkan_dev->hDevice, surfaceVK->hImage, nullptr);

		if (surfaceVK->hMemory != VK_NULL_HANDLE)
			vkFreeMemory(vulkan_dev->hDevice, surfaceVK->hMemory, nullptr);

		if (surfaceVK->Sync.hSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(vulkan_dev->hDevice, surfaceVK->Sync.hSemaphore, nullptr);
	}
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::copy_to_surface(AMFSurface *y, AMFSurface *uv, AMFSurface *out)
{
	AMFOBS_RETURN_IF_FALSE(amf_compute != nullptr, AMF_NOT_INITIALIZED,
			       "copy_to_surface: MFVulkanOBS not initialized");
	AMFOBS_RETURN_IF_FALSE(y != nullptr, AMF_INVALID_ARG, "copy_to_surface: y == nullptr");
	AMFOBS_RETURN_IF_FALSE(uv != nullptr, AMF_INVALID_ARG, "copy_to_surface: uv == nullptr");

	AMF_RESULT res;
	amf_size origin[3] = {0, 0, 0};
	amf_size regionY[3] = {(amf_size)y->GetPlaneAt(0)->GetWidth(), (amf_size)y->GetPlaneAt(0)->GetHeight(), 1};
	amf_size regionUV[3] = {(amf_size)uv->GetPlaneAt(0)->GetWidth(), (amf_size)uv->GetPlaneAt(0)->GetHeight(), 1};

	res = amf_compute->CopyPlane(y->GetPlaneAt(0), origin, regionY, out->GetPlaneAt(0), origin);
	AMFOBS_RETURN_IF_FAILED(res, "copy_to_surface: AMFCompute::CopyPlane(y) failed");

	res = amf_compute->CopyPlane(uv->GetPlaneAt(0), origin, regionUV, out->GetPlaneAt(1), origin);
	AMFOBS_RETURN_IF_FAILED(res, "copy_to_surface: AMFCompute::CopyPlane(uv) failed");
	// attach input AMF surfaces to the output surface to avoid their deletion till encoding is done.
	// allows not wait on CPU and comment next line
	//amf_compute->FinishQueue();

	out->SetProperty(L"OBS_SURFACE_Y", AMFInterfacePtr(y).GetPtr());
	out->SetProperty(L"OBS_SURFACE_UV", AMFInterfacePtr(uv).GetPtr());

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::copy_obs_to_amf(struct encoder_texture *texture, uint64_t /*lock_key*/,
					    uint64_t * /*next_key*/, amf::AMF_SURFACE_FORMAT format, int width,
					    int height, amf::AMFSurface **out)
{
	AMFOBS_RETURN_IF_FALSE(amf_context != nullptr, AMF_NOT_INITIALIZED, "amf_vulkan_impl not initialized");

	GLuint tex_y = *((GLuint *)gs_texture_get_obj(texture->tex[0]));
	GLuint tex_uv = *((GLuint *)gs_texture_get_obj(texture->tex[1]));

	AMF_RESULT res;
	AMFSurfacePtr y;
	AMFSurfacePtr uv;

	// sync OpenGL
	//glFinish();
	// simple sync : can be improved with wait on GPU using EXT_external_semaphore objects created in Vulkan and imported into OGL
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); // 1 second in nanoseconds
	glDeleteSync(sync);

	// import to AMF
	egl_image egl_y(this);
	egl_image egl_uv(this);

	res = egl_y.import_to_amf(tex_y, format, AMF_PLANE_Y, width, height, &y);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: import_to_amf() failed");

	res = egl_uv.import_to_amf(tex_uv, format, AMF_PLANE_UV, width / 2, height / 2, &uv);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: import_to_amf() failed");

	AMFSurfacePtr surface;
	res = amf_context->AllocSurface(AMF_MEMORY_VULKAN, format, y->GetPlaneAt(0)->GetWidth(),
					y->GetPlaneAt(0)->GetHeight(), &surface);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: AMFContext::AllocSurface() failed");

	// copy
	res = copy_to_surface(y, uv, surface);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: copy_to_surface() failed");

	*out = surface.Detach();

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::egl_image::import_to_amf(GLuint tex, AMF_SURFACE_FORMAT format, AMF_PLANE_TYPE planeType,
						     int width, int height, AMFSurface **out)
{
	egl_img = eglCreateImage(eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D,
				 (EGLClientBuffer)(uint64_t)tex, nullptr);
	AMFOBS_RETURN_IF_EGL_FAILED(AMF_FAIL, "egl_image:import_to_amf: eglCreateImage() failed %d)", eglGetError());

	EGLint stride = 0;
	EGLint offset = 0;
	amf_host->eglExportDMABUFImageMESA(eglGetCurrentDisplay(), egl_img, &fd, &stride, &offset);
	AMFOBS_RETURN_IF_EGL_FAILED(AMF_FAIL, "egl_image:import_to_amf: eglExportDMABUFImageMESA() failed %d)",
				    eglGetError());

	AMF_RESULT res = amf_host->create_surface_from_handle(fd, format, planeType, width, height, out);
	AMFOBS_RETURN_IF_FAILED(res, "egl_image:import_to_amf: create_surface_from_handle() failed");

	fd = 0; //ownership of FD transferred to Vulkan
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::get_luid(uint64_t *luid)
{
	AMFOBS_RETURN_IF_FALSE(amfvk_device.hPhysicalDevice != 0, AMF_NOT_INITIALIZED, "get_luid: Not initialized");

	VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
	VkPhysicalDeviceIDProperties physicalDeviceIDProperties = {};
	VkPhysicalDeviceDriverProperties driverProperties = {};
	driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

	physicalDeviceIDProperties.pNext = &driverProperties;
	physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

	physicalDeviceProperties.pNext = &physicalDeviceIDProperties;
	physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	vkGetPhysicalDeviceProperties2KHR(amfvk_device.hPhysicalDevice, &physicalDeviceProperties);
	// return  LUID
	*luid = *((uint64_t *)physicalDeviceIDProperties.deviceUUID);
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::get_vendor(uint32_t *vendor)
{
	AMFOBS_RETURN_IF_FALSE(amfvk_device.hPhysicalDevice != 0, AMF_NOT_INITIALIZED, "get_luid: Not initialized");

	VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
	VkPhysicalDeviceIDProperties physicalDeviceIDProperties = {};
	VkPhysicalDeviceDriverProperties driverProperties = {};
	driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

	physicalDeviceIDProperties.pNext = &driverProperties;
	physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

	physicalDeviceProperties.pNext = &physicalDeviceIDProperties;
	physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	vkGetPhysicalDeviceProperties2KHR(amfvk_device.hPhysicalDevice, &physicalDeviceProperties);
	// return  ID
	*vendor = physicalDeviceProperties.properties.vendorID;
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init_vulkan_device(AMFContext *context, uint32_t adapter)
{
	// this code borrowed from AMF sample DeviceVulkan.cpp

	VkResult vkres = VK_SUCCESS;
	std::vector<VkPhysicalDevice> physicalDevices;
	std::vector<const char *> deviceExtensions;

	amfvk_device.cbSizeof = sizeof(amfvk_device);

	amf::AMFContext1Ptr pContext1(context);
	amf_size nCount = 0;
	pContext1->GetVulkanDeviceExtensions(&nCount, NULL);
	if (nCount > 0) {
		deviceExtensions.resize(nCount);
		pContext1->GetVulkanDeviceExtensions(&nCount, deviceExtensions.data());
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	std::vector<const char *> instanceExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		/*
#if defined(_WIN32)
        	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__ANDROID__)
        	VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined(__linux)
        	VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
*/
	};
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());

	std::vector<const char *> instanceLayers;

	instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());

	// VkApplicationInfo
	///////////////////////
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.apiVersion = VK_API_VERSION_1_3;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pApplicationName = "AMF Vulkan application";
	applicationInfo.pEngineName = "AMD Vulkan Sample Engine";

	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	vkres = vkCreateInstance(&instanceCreateInfo, nullptr, &amfvk_device.hInstance);
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "init_vulkan_device: CreateInstance() failed to vkCreateInstance, Error=%d", (int)vkres);

	GET_INSTANCE_ENTRYPOINT(amfvk_device.hInstance, vkGetPhysicalDeviceProperties2KHR);

	uint32_t physicalDeviceCount = 0;
	vkres = vkEnumeratePhysicalDevices(amfvk_device.hInstance, &physicalDeviceCount, nullptr);
	AMFOBS_RETURN_IF_FALSE(
		vkres == VK_SUCCESS, AMF_FAIL,
		"init_vulkan_device: GetPhysicalDevices() failed to vkEnumeratePhysicalDevices, Error=%d", (int)vkres);

	physicalDevices.clear();
	physicalDevices.resize(physicalDeviceCount);

	vkres = vkEnumeratePhysicalDevices(amfvk_device.hInstance, &physicalDeviceCount, physicalDevices.data());
	AMFOBS_RETURN_IF_FALSE(
		vkres == VK_SUCCESS, AMF_FAIL,
		"init_vulkan_device: GetPhysicalDevices() failed to vkEnumeratePhysicalDevices, Error=%d", (int)vkres);

	// test code
	/*
	for(uint32_t i =0 ; i < (uint32_t)physicalDevices.size(); i++ )
	{
		uint64_t luid = 0;
		VkPhysicalDeviceProperties2  physicalDeviceProperties   = {};
		VkPhysicalDeviceIDProperties physicalDeviceIDProperties = {};
		VkPhysicalDeviceDriverProperties driverProperties = {};
		driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

		physicalDeviceIDProperties.pNext = &driverProperties;
		physicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

		physicalDeviceProperties.pNext = &physicalDeviceIDProperties;
		physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2KHR(physicalDevices[i], &physicalDeviceProperties);
		// return  LUID
		luid  = *((uint64_t*)physicalDeviceIDProperties.deviceUUID);
		(void)luid;
	}
*/
	////

	AMFOBS_RETURN_IF_FALSE(adapter < (uint32_t)physicalDevices.size(), AMF_NO_DEVICE,
			       "init_vulkan_device: Invalid Adapter ID=%d", (int)adapter);

	amfvk_device.hPhysicalDevice = physicalDevices[adapter];

	// find and request queues

	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfoItems;
	std::vector<float> queuePriorities;

	uint32_t queueFamilyPropertyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties2(amfvk_device.hPhysicalDevice, &queueFamilyPropertyCount, nullptr);

	AMFOBS_RETURN_IF_FALSE(queueFamilyPropertyCount != 0, AMF_FAIL,
			       "init_vulkan_device: CreateDeviceAndQueues() queueFamilyPropertyCount = 0");

	std::vector<VkQueueFamilyProperties2> queueFamilyProperties;
	queueFamilyProperties.resize(queueFamilyPropertyCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
	vkGetPhysicalDeviceQueueFamilyProperties2(amfvk_device.hPhysicalDevice, &queueFamilyPropertyCount,
						  queueFamilyProperties.data());

	amf_uint32 uQueueGraphicsFamilyIndex = UINT32_MAX;
	amf_uint32 uQueueComputeFamilyIndex = UINT32_MAX;
	amf_uint32 uQueueDecodeFamilyIndex = UINT32_MAX;

	amf_uint32 uQueueGraphicsIndex = UINT32_MAX;
	amf_uint32 uQueueComputeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
		VkQueueFamilyProperties2 &queueFamilyProperty = queueFamilyProperties[i];
		if (queuePriorities.size() < queueFamilyProperty.queueFamilyProperties.queueCount) {
			queuePriorities.resize(queueFamilyProperty.queueFamilyProperties.queueCount, 1.0f);
		}
	}
	for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
		VkQueueFamilyProperties2 &queueFamilyProperty = queueFamilyProperties[i];
		VkDeviceQueueCreateInfo queueCreateInfo = {};

		queueCreateInfo.pQueuePriorities = &queuePriorities[0];
		queueCreateInfo.queueFamilyIndex = i;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		if ((queueFamilyProperty.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
		    (queueFamilyProperty.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
		    uQueueComputeFamilyIndex == UINT32_MAX) {
			uQueueComputeFamilyIndex = i;
			uQueueComputeIndex = 0;
			deviceQueueCreateInfoItems.push_back(queueCreateInfo);
		}
		if ((queueFamilyProperty.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		    uQueueGraphicsFamilyIndex == UINT32_MAX) {
			uQueueGraphicsFamilyIndex = i;
			uQueueGraphicsIndex = 0;
			deviceQueueCreateInfoItems.push_back(queueCreateInfo);
		}
		if ((queueFamilyProperty.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) &&
		    uQueueDecodeFamilyIndex == UINT32_MAX) {
			uQueueDecodeFamilyIndex = i;
			deviceQueueCreateInfoItems.push_back(queueCreateInfo);
		}
	}
	// create device
	VkDeviceCreateInfo deviceCreateInfo = {};

	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfoItems.size());
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfoItems[0];

	VkPhysicalDeviceFeatures features = {};
	vkGetPhysicalDeviceFeatures(amfvk_device.hPhysicalDevice, &features);
	deviceCreateInfo.pEnabledFeatures = &features;

	VkPhysicalDeviceSynchronization2Features syncFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
	syncFeatures.synchronization2 = VK_TRUE;
	deviceCreateInfo.pNext = &syncFeatures;

	VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD};
	coherentMemoryFeatures.deviceCoherentMemory = VK_TRUE;
	syncFeatures.pNext = &coherentMemoryFeatures;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
	coherentMemoryFeatures.pNext = &indexingFeatures;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;

	VkPhysicalDeviceShaderAtomicInt64FeaturesKHR atomicInt64Features = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR};
	atomicInt64Features.shaderBufferInt64Atomics = VK_TRUE;
	indexingFeatures.pNext = &atomicInt64Features;

	std::vector<const char *> SupportDeviceExtensions;
	uint32_t extensionCount = 0;

	vkres = vkEnumerateDeviceExtensionProperties(amfvk_device.hPhysicalDevice, nullptr, &extensionCount, NULL);
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "init_vulkan_device: vkEnumerateDeviceExtensionProperties() failed, err = %d", vkres);
	std::vector<VkExtensionProperties> deviceExtensionProperties(extensionCount);
	vkres = vkEnumerateDeviceExtensionProperties(amfvk_device.hPhysicalDevice, nullptr, &extensionCount,
						     deviceExtensionProperties.data());
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "init_vulkan_device: vkEnumerateDeviceExtensionProperties() failed, err = %d", vkres);

	// convert to hash, could use string_view if available
	std::unordered_set<std::string> deviceExtensionLookup;
	for (std::vector<VkExtensionProperties>::iterator it = deviceExtensionProperties.begin();
	     it != deviceExtensionProperties.end(); it++) {
		deviceExtensionLookup.insert(it->extensionName);
	}
	for (std::vector<const char *>::iterator it = deviceExtensions.begin(); it != deviceExtensions.end(); it++) {
		if (deviceExtensionLookup.find(*it) != deviceExtensionLookup.end()) {
			SupportDeviceExtensions.push_back(*it);
		} else {
			warn("init_vulkan_device: Extension %s is not available. Some Vulkan features may not work correctly.",
			     (*it));
		}
	}

	deviceCreateInfo.ppEnabledExtensionNames = SupportDeviceExtensions.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(SupportDeviceExtensions.size());

	vkres = vkCreateDevice(amfvk_device.hPhysicalDevice, &deviceCreateInfo, nullptr, &amfvk_device.hDevice);

	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL,
			       "init_vulkan_device: CreateDeviceAndQueues() vkCreateDevice() failed, Error=%d",
			       (int)vkres);
	AMFOBS_RETURN_IF_FALSE(amfvk_device.hDevice != nullptr, AMF_FAIL,
			       "init_vulkan_device: CreateDeviceAndQueues() vkCreateDevice() returned nullptr");

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
