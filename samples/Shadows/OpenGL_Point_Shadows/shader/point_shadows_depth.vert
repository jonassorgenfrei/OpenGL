#version 330 core
/**
 * Simply transforms vertices to world-space
 */
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main() 
{
	gl_Position = model * vec4(aPos, 1.0);
}