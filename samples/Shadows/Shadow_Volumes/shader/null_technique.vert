#version 330

layout (location = 0) in vec3 Position; 

uniform mat4 mvp;

// Multipass rendering requires the depth pre-pass and lighting pass to produce
// exactly the same clip-space position for every vertex.
invariant gl_Position;

void main()
{
    // Both passes receive the same CPU-computed matrix and execute the same
    // operation, avoiding cross-program floating-point differences.
    gl_Position = mvp * vec4(Position, 1.0);
}
