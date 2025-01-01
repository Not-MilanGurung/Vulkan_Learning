# Compiling the vertex shader from glsl to spir-v tecode
glslc vertex_shader.vert -o vert.spv 
# Compiling the fragment shader from glsl to spir-v bytecode
glslc fragment_shader.frag -o frag.spv

# This compiling can be done in the c++ program using libshaderc