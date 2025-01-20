#version 460 // Version of shader preprocessor

// Global variables for input and output
layout(binding = 1) uniform sampler2D texSampler;
// Taking in the fragColor from the vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
// Specifying the index of the frame buffer
// Here the index is 0
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}