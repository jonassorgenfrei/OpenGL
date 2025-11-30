#version 430 core
// Final blit shader: copies a texture to the framebuffer.


// shader inputs
in vec2 texture_coords;

// shader outputs
layout (location = 0) out vec4 frag;

// screen image
uniform sampler2D screenTex;

void main()
{
	frag = texture(screenTex, texture_coords);
}

