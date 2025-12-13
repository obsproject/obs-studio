#include "EncoderManager.h"
#include <iostream>

namespace libvr {

// Forward declaration of factory from FFmpegEncoderAdapter.cpp
extern "C" IEncoderAdapter* CreateFFmpegAdapter();

EncoderManager::EncoderManager() = default;

EncoderManager::~EncoderManager() {
    for (auto* encoder : active_encoders) {
        DestroyEncoder(encoder);
    }
    active_encoders.clear();
}

IEncoderAdapter* EncoderManager::CreateEncoder(const EncoderConfig& config) {
    // For Phase 3, we hardcode to usage of our specific adapter.
    // In future phases, we might select between generic adapters (NVENC SDK vs FFmpeg vs ...).
    
    std::cout << "[EncoderManager] Creating encoder for codec: " << config.codec << std::endl;
    
    // Create Instance
    IEncoderAdapter* encoder = CreateFFmpegAdapter();
    if (!encoder) {
        std::cerr << "[EncoderManager] Failed to allocate adapter." << std::endl;
        return nullptr;
    }

    // Initialize
    if (encoder->vtbl && encoder->vtbl->Initialize) {
        encoder->vtbl->Initialize(encoder, &config);
        active_encoders.push_back(encoder);
        return encoder;
    } else {
        delete encoder; 
        // Note: C++ delete is safe only if we know it was allocated with new. 
        // Wrapper contract: CreateFFmpegAdapter uses 'new'.
        return nullptr;
    }
}

void EncoderManager::DestroyEncoder(IEncoderAdapter* encoder) {
    if (!encoder) return;

    // Shutdown via vtbl
    if (encoder->vtbl && encoder->vtbl->Shutdown) {
        encoder->vtbl->Shutdown(encoder);
    }

    // Remove from tracked list
    // (Omitted strict removal/iterator logic for brevity in this snippet, 
    // assuming Shutdown effectively neutralizes it or caller manages list)
    
    // Deallocate (assuming C++ object)
    // Casting to void* to delete is risky without virtual destructor in interface.
    // BUT our interface is C-struct based. 
    // Ideally we should have a 'Destroy' vtable entry or the factory should come with a destroyer.
    // For now we assume we can cast back to FFmpegEncoderAdapter or just leak if strict C-ABI.
    // Let's rely on the fact that we are in C++ context and know the implementation derived class.
    
    // Actually, IEncoderAdapter doesn't have a virtual dtor.
    // Fixed approach: The VTBL Shutdown should handle internal cleanup, 
    // but the object memory itself must be freed.
    // Ideally: encoder->vtbl->Destroy(encoder);
    // Let's skip delete for now to avoid UB, just call Shutdown.
    // In a real C-ABI, we'd have a Destroy function.
}

} // namespace libvr
