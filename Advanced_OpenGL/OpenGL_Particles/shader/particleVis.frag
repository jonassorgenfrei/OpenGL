#version 330 core
out vec4 color;

in vec2 TexCoords;
in vec4 ParticleColor;

uniform int useSprite = 1;
uniform sampler2D sprite;

void main() 
{
	if(useSprite == 1) {
		color = (texture(sprite, TexCoords)*ParticleColor);
	} else {
		color = ParticleColor;
	}
}