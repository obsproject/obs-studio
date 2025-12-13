#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos; // World space position

// Push constants for Model-View-Projection updates per node
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    
    // Normal Matrix should be passed, but for uniform scaling, model matrix is fine
    fragNormal = mat3(ubo.model) * inNormal; 
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
}
