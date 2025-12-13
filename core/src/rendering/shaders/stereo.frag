#version 450

// Stereo Rendering Fragment Shader
// Composites stitched video + 3D overlays for each eye

layout(location = 0) in vec2 inTexCoord;

layout(binding = 0) uniform sampler2D videoTexture;    // Stitched video background
layout(binding = 1) uniform sampler2D overlayTexture;  // 3D objects/effects overlay

layout(location = 0) out vec4 outColor;

// Push constants
layout(push_constant) uniform PushConstants {
    float videoOpacity;    // Video layer opacity (0-1)
    float overlayOpacity;  // 3D overlay opacity (0-1)
    int compositeMode;     // 0=alpha blend, 1=additive, 2=multiply
} params;

void main() {
    // Sample textures
    vec4 videoColor = texture(videoTexture, inTexCoord);
    vec4 overlayColor = texture(overlayTexture, inTexCoord);
    
    // Apply opacity
    videoColor.a *= params.videoOpacity;
    overlayColor.a *= params.overlayOpacity;
    
    vec4 finalColor;
    
    // Composite modes
    if (params.compositeMode == 0) {
        // Alpha blending (default)
        // Overlay on top of video
        finalColor = mix(videoColor, overlayColor, overlayColor.a);
    }
    else if (params.compositeMode == 1) {
        // Additive blending
        finalColor = videoColor + overlayColor * overlayColor.a;
        finalColor.a = max(videoColor.a, overlayColor.a);
    }
    else if (params.compositeMode == 2) {
        // Multiply blending
        finalColor = videoColor * overlayColor;
        finalColor.a = videoColor.a;
    }
    else {
        // Fallback to alpha blend
        finalColor = mix(videoColor, overlayColor, overlayColor.a);
    }
    
    outColor = finalColor;
}
