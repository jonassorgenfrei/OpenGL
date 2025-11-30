#version 430 core
// Passthrough vertex shader applying MVP for opaque geometry.


// shader inputs
layout (location = 0) in vec3 position;

// mvp matrix
uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(position, 1.0f);
}

