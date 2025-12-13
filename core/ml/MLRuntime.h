#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward decl to avoid including onnxruntime_c_api.h in header if possible, 
// but usually we need the types unless we pimpl. 
// For simplicity, we'll expose a clean C++ API and hide implementation.

namespace libvr {

class MLRuntime {
public:
    MLRuntime();
    ~MLRuntime();

    bool Initialize();
    bool LoadModel(const std::string& model_path);
    
    // Simple inference: float input -> float output
    // Phase 4: CPU copy. Phase 5: Hardware buffer.
    bool RunInference(const std::vector<float>& input, std::vector<float>& output);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace libvr
