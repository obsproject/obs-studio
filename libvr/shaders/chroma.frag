#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

// Chroma Key Settings (could be UBO)
layout(binding = 1) uniform ChromaSettings {
    vec4 keyColor;      // RGB + Threshold (w)
    float smoothness;   // Soft edge
    float spill;        // Spill reduction
    int enabled;        // Toggle
} chroma;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);

    if (chroma.enabled == 1) {
        // Simple Euclidean Distance Keying
        // Future: Convert RGB -> Chroma (YUV/Lab) for better quality
        
        vec3 key = chroma.keyColor.rgb;
        float threshold = chroma.keyColor.w; // stored in alpha of keyColor
        
        float dist = distance(color.rgb, key);
        
        if (dist < threshold) {
            discard; // Fully transparent
            // Or soft alpha:
            // float alpha = smoothstep(threshold, threshold + chroma.smoothness, dist);
            // color.a *= alpha;
        } else if (dist < threshold + chroma.smoothness) {
             // Edge smoothing
             float alpha = smoothstep(threshold, threshold + chroma.smoothness, dist);
             color.a *= alpha;
        }
    }
    
    outColor = color;
}
