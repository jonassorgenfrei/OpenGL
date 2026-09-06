#version 430 core

// Shader inputs
in vec2 texture_coords;

// Shader outputs
layout (location = 0) out vec4 frag;

// screen image
uniform sampler2D screen;

void main()
{
	frag = vec4(texture(screen, texture_coords).rgb, 1.0f);
}

