#version 430 core

// Shader inputs
layout (location = 0) in vec3 position;

// mvp matrix
uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(position, 1.0f);
}

