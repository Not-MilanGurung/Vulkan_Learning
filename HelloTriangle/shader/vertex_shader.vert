#version 460 // Version of shader preprocessor

// Global variables for input and output

layout(location = 0) out vec3 fragColor;

// Position of the three vertices of the triangle
// Here 0,0 is the center and the values range from -1.0 to 1.0 
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5), 
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

// Colors of the vetex
// Creating a gradient
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);


void main() {
    // gl_VertexIndex is a built in variable that holds the value of the current index
    // The main function is run for every vertex
    // The last values 0.0, 1.0 and dummy z and w components 
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // Giving the color of the vertices to the global output variable
    fragColor = colors[gl_VertexIndex];
}