#version 460 // Version of shader preprocessor

// Global variables for input and output

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // gl_VertexIndex is a built in variable that holds the value of the current index
    // The main function is run for every vertex
    // The last values 0.0, 1.0 and dummy z and w components 
    // A MVP transformation is done with the uniform buffer object
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // Giving the color of the vertices to the global output variable
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}