#version 430 core
// Vertex shader for transparent quads; forwards clip-space position.


// shader inputs

layout (location = 0) in vec3 position;
out vec4 fragPos; // clip-space position
// model * view * projection matrix
uniform mat4 mvp;

void main()
{
	fragPos = mvp * vec4(position, 1.0f);
	gl_Position = mvp * vec4(position, 1.0f);
}
