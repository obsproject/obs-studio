#version 450

// Stereo Rendering Vertex Shader
// Full-screen quad for stereo compositing

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main() {
    outTexCoord = inTexCoord;
    gl_Position = vec4(inPosition, 1.0);
}
