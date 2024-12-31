#version 460 // Version of shader preprocessor

// Global variables for input and output

// Taking in the fragColor from the vertex shader
layout(location = 0) in vec3 fragColor;
// Specifying the index of the frame buffer
// Here the index is 0
layout(location = 0) out vec4 outColor;

void main() {
    // Here red, blue, green, aplha channel are defined in range 0-1
    // Using the fragColor defined in the vertex shader
    outColor = vec4(fragColor, 1.0);
}