#if defined(linux) || defined(unix) || defined(__linux)
  #include <dlfcn.h>
  #include <string>
  #include <cstdio>
  #include <cstring>
  #include <cstdlib>
  #include "include/nvVideoEffects.h"

  #define NvVFX_API
  
  static void* NvVfxLibHandle = nullptr;

  static void* getNvVfxLib() {
      if (NvVfxLibHandle) return NvVfxLibHandle;
      
      const char* env_path = getenv("NV_VIDEO_EFFECTS_PATH");
      // std::string libPath = env_path ? std::string(env_path) + "/libNVVideoEffects.so" : "/usr/local/NVIDIA/VideoEffects/libNVVideoEffects.so" ;
      // Actually usually it's in /usr/lib or LD_LIBRARY_PATH if installed
      // Try loading directly first
      NvVfxLibHandle = dlopen("libNVVideoEffects.so", RTLD_LAZY | RTLD_GLOBAL);
      if (!NvVfxLibHandle && env_path) {
          std::string path = std::string(env_path) + "/libNVVideoEffects.so";
          NvVfxLibHandle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
      }
      return NvVfxLibHandle;
  }
  
  static void* nvGetProcAddress(void* handle, const char* proc) {
      if (!handle) return nullptr;
      return dlsym(handle, proc);
  }

extern "C" {

  NvCV_Status NvVFX_API NvVFX_GetVersion(unsigned int* version) {
    static const auto funcPtr = (NvCV_Status (*)(unsigned int*))nvGetProcAddress(getNvVfxLib(), "NvVFX_GetVersion");
    if (!funcPtr) return NVCV_ERR_LIBRARY;
    return funcPtr(version);
  }

  NvCV_Status NvVFX_API NvVFX_CreateEffect(NvVFX_EffectSelector code, NvVFX_Handle* obj) {
    static const auto funcPtr = (NvCV_Status (*)(NvVFX_EffectSelector, NvVFX_Handle*))nvGetProcAddress(getNvVfxLib(), "NvVFX_CreateEffect");
    if (!funcPtr) return NVCV_ERR_LIBRARY;
    return funcPtr(code, obj);
  }

  void NvVFX_API NvVFX_DestroyEffect(NvVFX_Handle obj) {
    static const auto funcPtr = (void (*)(NvVFX_Handle))nvGetProcAddress(getNvVfxLib(), "NvVFX_DestroyEffect");
    if (funcPtr) funcPtr(obj);
  }

  NvCV_Status NvVFX_API NvVFX_SetImage(NvVFX_Handle obj, NvVFX_ParameterSelector paramName, NvCVImage* im) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, NvVFX_ParameterSelector, NvCVImage*))nvGetProcAddress(getNvVfxLib(), "NvVFX_SetImage");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, paramName, im);
  }
  
  NvCV_Status NvVFX_API NvVFX_SetString(NvVFX_Handle obj, NvVFX_ParameterSelector paramName, const char* str) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, NvVFX_ParameterSelector, const char*))nvGetProcAddress(getNvVfxLib(), "NvVFX_SetString");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, paramName, str);
  }

  NvCV_Status NvVFX_API NvVFX_SetU32(NvVFX_Handle obj, NvVFX_ParameterSelector paramName, unsigned int val) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, NvVFX_ParameterSelector, unsigned int))nvGetProcAddress(getNvVfxLib(), "NvVFX_SetU32");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, paramName, val);
  }
  
  NvCV_Status NvVFX_API NvVFX_SetF32(NvVFX_Handle obj, NvVFX_ParameterSelector paramName, float val) {
       static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, NvVFX_ParameterSelector, float))nvGetProcAddress(getNvVfxLib(), "NvVFX_SetF32");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, paramName, val);
  }

  NvCV_Status NvVFX_API NvVFX_SetCudaStream(NvVFX_Handle obj, NvVFX_ParameterSelector paramName, void* stream) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, NvVFX_ParameterSelector, void*))nvGetProcAddress(getNvVfxLib(), "NvVFX_SetCudaStream");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, paramName, stream);
  }

  NvCV_Status NvVFX_API NvVFX_Load(NvVFX_Handle obj) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle))nvGetProcAddress(getNvVfxLib(), "NvVFX_Load");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj);
  }

  NvCV_Status NvVFX_API NvVFX_Run(NvVFX_Handle obj, int async) {
      static const auto funcPtr = (NvCV_Status (*)(NvVFX_Handle, int))nvGetProcAddress(getNvVfxLib(), "NvVFX_Run");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(obj, async);
  }
  
  NvCV_Status NvVFX_API NvVFX_CudaStreamCreate(void** stream) {
      static const auto funcPtr = (NvCV_Status (*)(void**))nvGetProcAddress(getNvVfxLib(), "NvVFX_CudaStreamCreate");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(stream);
  }
  
  NvCV_Status NvVFX_API NvVFX_CudaStreamDestroy(void* stream) {
      static const auto funcPtr = (NvCV_Status (*)(void*))nvGetProcAddress(getNvVfxLib(), "NvVFX_CudaStreamDestroy");
      if (!funcPtr) return NVCV_ERR_LIBRARY;
      return funcPtr(stream);
  }

} // extern C
#endif
