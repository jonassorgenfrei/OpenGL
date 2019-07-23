#version 330

in vec3 ViewPos;

layout (location = 0) out vec3 PosOut;

void main() {
	PosOut = ViewPos; // write view space position to the texture
}