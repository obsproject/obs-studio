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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define AMFOBS_ENCODER "amf_encoder"
#define AMFOBS_ENCODER_NAME "amf_vulkan_impl"

#include "amf_gpu.hpp"

using namespace amf;

#define AMFOBS_RETURN_IF_EGL_FAILED(_ret, format, ...) \
	AMFOBS_RETURN_IF_FALSE((eglGetError() == EGL_SUCCESS), _ret, format, ##__VA_ARGS__)

#define GET_DLL_ENTRYPOINT(h, w) w = reinterpret_cast<PFN_##w>(dlsym(h, #w)); if(w==nullptr) \
	{ error("Failed to aquire entrypoint %s", #w); return AMF_FAIL; };

class amf_vulkan_impl : public amf_gpu, public amf::AMFSurfaceObserver {
public:
	amf_vulkan_impl() {}

	AMF_RESULT init(amf::AMFContext *context, uint32_t adapter) override;
	AMF_RESULT terminate() override;
	AMF_RESULT copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
				   amf::AMF_SURFACE_FORMAT format, int width, int height,
				   amf::AMFSurface **surface) override;

private:
	// AMFSurfaceObserver interface
	virtual void AMF_STD_CALL OnSurfaceDataRelease(amf::AMFSurface *pSurface);

	// helpers
	AMF_RESULT create_surface_from_handle(int fd, amf::AMF_SURFACE_FORMAT format, amf::AMF_PLANE_TYPE planeType,
					      int width, int height, amf::AMFSurface **surface);
	AMF_RESULT copy_to_surface(amf::AMFSurface *y, amf::AMFSurface *uv, amf::AMFSurface *out);
	AMF_RESULT init_opengl();

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
	// EGL & OGL functions

	PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA = nullptr;
	PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA = nullptr;
	PFNGLFENCESYNCPROC glFenceSync = nullptr;
	PFNGLCLIENTWAITSYNCPROC glClientWaitSync = nullptr;
	PFNGLDELETESYNCPROC glDeleteSync = nullptr;

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
		AMF_RESULT import_to_amf(GLuint tex, AMF_SURFACE_FORMAT format, int width, int height,
					 AMFSurface **out);

	protected:
		amf_vulkan_impl *amf_host = nullptr;
		EGLImage egl_img = 0;
		int fd = 0;
	};
};

//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_gpu::create(amf_gpu **amf_gpu_obj)
{
	*amf_gpu_obj = new amf_vulkan_impl();
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init(AMFContext *context, uint32_t /*adapter*/)
{
	AMF_RESULT res = AMFContext2Ptr(context)->InitVulkan(nullptr);
	AMFOBS_RETURN_IF_FAILED(res, "AMFContext2::InitVulkan failed with res = %d", res);

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

	res = context->GetCompute(AMF_MEMORY_VULKAN, &amf_compute);
	AMFOBS_RETURN_IF_FAILED(res, "AMFContext2::GetCompute(AMF_MEMORY_VULKAN) failed with res = %d", res);

	res = init_opengl();
	AMFOBS_RETURN_IF_FAILED(res, "init_opengl failed with res = %d", res);

	amf_context = AMFContext2Ptr(context);
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::init_opengl()
{
	eglExportDMABUFImageQueryMESA =
		(PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)eglGetProcAddress("eglExportDMABUFImageQueryMESA");
	eglExportDMABUFImageMESA = (PFNEGLEXPORTDMABUFIMAGEMESAPROC)eglGetProcAddress("eglExportDMABUFImageMESA");

	AMFOBS_RETURN_IF_FALSE(eglExportDMABUFImageMESA != nullptr, AMF_NOT_SUPPORTED,
			       "egl doesn't support eglExportDMABUFImageMESA");
	// prepare OGL for sync

	static const char *NAMES[] = {"libGL.so.1", "libGL.so"};

	typedef void *(APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char *);
	PFNGLXGETPROCADDRESSPROC_PRIVATE glGetProcAddressPtr = nullptr;

	for (unsigned int index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
		opengl_so = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL);

		if (opengl_so != NULL)
			glGetProcAddressPtr =
				(PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(opengl_so, "glXGetProcAddressARB");

		if (glGetProcAddressPtr != nullptr)
			break;
	}
	AMFOBS_RETURN_IF_FALSE(glGetProcAddressPtr != nullptr, AMF_NOT_SUPPORTED,
			       "gl doesn't support glXGetProcAddressARB");

	glFenceSync = (PFNGLFENCESYNCPROC)glGetProcAddressPtr("glFenceSync");
	glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)glGetProcAddressPtr("glClientWaitSync");
	glDeleteSync = (PFNGLDELETESYNCPROC)glGetProcAddressPtr("glDeleteSync");

	AMFOBS_RETURN_IF_FALSE(glFenceSync != nullptr, AMF_NOT_SUPPORTED, "gl doesn't support glFenceSync");
	AMFOBS_RETURN_IF_FALSE(glClientWaitSync != nullptr, AMF_NOT_SUPPORTED, "gl doesn't support glClientWaitSync");
	AMFOBS_RETURN_IF_FALSE(glDeleteSync != nullptr, AMF_NOT_SUPPORTED, "gl doesn't support glDeleteSync");

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::terminate()
{
	amf_compute = nullptr;
	amf_context = nullptr;
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
	std::unique_ptr<amf_uint8> surface_data(new amf_uint8[sizeof(AMFVulkanSurface) + sizeof(AMFVulkanSurface1)]);

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
	imageCreateInfo.arrayLayers = 1;

	//	uint32_t computeQueueIndex =  amf_compute->GetNativeCommandQueue();
	//	imageCreateInfo.pQueueFamilyIndices = &computeQueueIndex;
	//	imageCreateInfo.queueFamilyIndexCount = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //VK_SHARING_MODE_CONCURRENT
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
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL, "CreateSurface() vkCreateImage() failed, err = %d",
			       vkres);

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
	AMFOBS_RETURN_IF_FALSE(memoryTypeIndex != UINT32_MAX, AMF_FAIL, "AllocateMemory() No memory request is found");

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
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL, "AllocateMemory() failed to vkAllocateMemory, Error = %d",
			       (int)vkres);
	AMFOBS_RETURN_IF_FALSE(surfaceVK->hMemory != VK_NULL_HANDLE, AMF_FAIL,
			       "AllocateMemory() cannot allocate memory");

	vkres = vkBindImageMemory(vulkan_dev->hDevice, surfaceVK->hImage, surfaceVK->hMemory, 0);

	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL, "vkBindImageMemory() failed, err = %d", vkres);

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
	AMFOBS_RETURN_IF_FALSE(vkres == VK_SUCCESS, AMF_FAIL, "vkCreateSemaphore() failed with %d", (int)vkres);

	AMF_RESULT res = amf_context->CreateSurfaceFromVulkanNative(surfaceVK, surface, this);
	AMFOBS_RETURN_IF_FAILED(res, "CreateSurfaceFromVulkanNative() failed");

	// success
	surface_data.release(); // will be deleted in callback

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
void AMF_STD_CALL amf_vulkan_impl::OnSurfaceDataRelease(AMFSurface *pSurface)
{
	if (pSurface != nullptr) {
		pSurface->RemoveObserver(this);
		if (amf_compute != nullptr) {
			AMFVulkanDevice *vulkan_dev = (AMFVulkanDevice *)amf_compute->GetNativeContext();
			AMFVulkanView *view = (AMFVulkanView *)pSurface->GetPlaneAt(0)->GetNative();
			std::unique_ptr<amf_uint8> surface_data((amf_uint8 *)view->pSurface); // aut-delete

			AMFVulkanSurface *surfaceVK = (AMFVulkanSurface *)surface_data.get();

			if (surfaceVK->hImage != VK_NULL_HANDLE)
				vkDestroyImage(vulkan_dev->hDevice, surfaceVK->hImage, nullptr);

			if (surfaceVK->hMemory != VK_NULL_HANDLE)
				vkFreeMemory(vulkan_dev->hDevice, surfaceVK->hMemory, nullptr);

			if (surfaceVK->Sync.hSemaphore != VK_NULL_HANDLE)
				vkDestroySemaphore(vulkan_dev->hDevice, surfaceVK->Sync.hSemaphore, nullptr);
		}
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
	amf_compute->FinishQueue();
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::copy_obs_to_amf(struct encoder_texture *texture, uint64_t /*lock_key*/,
					    uint64_t * /*next_key*/, amf::AMF_SURFACE_FORMAT format, int width,
					    int height, amf::AMFSurface **out)
{

	GLuint tex_y = *((GLuint *)gs_texture_get_obj(texture->tex[0]));
	GLuint tex_uv = *((GLuint *)gs_texture_get_obj(texture->tex[1]));

	AMF_RESULT res;
	AMFSurfacePtr y;
	AMFSurfacePtr uv;

	// interop to FD

	//glFinish();
	// simple sync : can be improved with wait on GPU using EXT_external_semaphore objects created in Vulkan and imported into OGL
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000); // 1 second in nanoseconds
	glDeleteSync(sync);

	egl_image egl_y(this);
	egl_image egl_uv(this);

	res = egl_y.import_to_amf(tex_y, format, width, height, &y);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: import_to_amf() failed");

	res = egl_uv.import_to_amf(tex_uv, format, width, height, &uv);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: import_to_amf() failed");

	AMFSurfacePtr surface;
	res = amf_context->AllocSurface(AMF_MEMORY_VULKAN, format, y->GetPlaneAt(0)->GetWidth(),
					y->GetPlaneAt(0)->GetHeight(), &surface);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: AMFContext::AllocSurface() failed");

	res = copy_to_surface(y, uv, surface);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: copy_to_surface() failed");

	*out = surface.Detach();

	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
AMF_RESULT amf_vulkan_impl::egl_image::import_to_amf(GLuint tex, AMF_SURFACE_FORMAT format, int width, int height,
						     AMFSurface **out)
{
	egl_img = eglCreateImage(eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_GL_TEXTURE_2D,
				 (EGLClientBuffer)(uint64_t)tex, nullptr);
	AMFOBS_RETURN_IF_EGL_FAILED(AMF_FAIL, "copy_obs_to_amf:egl_image: eglCreateImage() failed %d)", eglGetError());

	EGLint stride = 0;
	EGLint offset = 0;
	amf_host->eglExportDMABUFImageMESA(eglGetCurrentDisplay(), egl_img, &fd, &stride, &offset);
	AMFOBS_RETURN_IF_EGL_FAILED(AMF_FAIL, "copy_obs_to_amf:egl_image:eglExportDMABUFImageMESA() failed %d)",
				    eglGetError());

	AMF_RESULT res = amf_host->create_surface_from_handle(fd, format, AMF_PLANE_Y, width, height, out);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf:egl_image: create_surface_from_handle() failed");

	fd = 0; //ownership of FD transferred to Vulkan
	return AMF_OK;
}
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
