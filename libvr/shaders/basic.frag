#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

// Global Lighting UBO ( Set 1 )
layout(set = 1, binding = 0) uniform LightUBO {
    vec4 lightPos;    // xyz, w=type
    vec4 lightColor;  // rgb, w=intensity
    vec4 viewPos;
} ubo;

void main() {
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor.rgb;
  
    // Diffuse 
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos);
    
    // If directional (type=1), lightPos is actually direction
    if (ubo.lightPos.w > 0.5) {
        lightDir = normalize(-ubo.lightPos.xyz);
    }

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor.rgb * ubo.lightColor.w;
    
    // Texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    outColor = vec4(result, texColor.a);
}
