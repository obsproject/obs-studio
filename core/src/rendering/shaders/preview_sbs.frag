#version 450

// Preview Fragment Shader - SBS Left Eye Extraction
// Extracts left half from side-by-side stereo input for clean 2D preview

layout(location = 0) in vec2 inTexCoord;

layout(binding = 0) uniform sampler2D inputTexture;  // Source texture (possibly SBS)

layout(location = 0) out vec4 outColor;

// Push constants
layout(push_constant) uniform PushConstants {
    int inputMode;    // 0 = mono, 1 = SBS (side-by-side)
    float brightness; // Brightness adjustment
} params;

void main() {
    vec2 uv = inTexCoord;
    
    if (params.inputMode == 1) {
        // SBS mode: sample from left half only
        // Map UV.x from [0, 1] to [0, 0.5]
        // This extracts left eye regardless of input resolution
        uv.x *= 0.5;
    }
    // else: mono mode, use UV as-is
    
    vec4 color = texture(inputTexture, uv);
    
    // Apply brightness adjustment
    color.rgb *= params.brightness;
    
    outColor = color;
}
