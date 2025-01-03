#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform vec3 lightColor;


void main()
{
	FragColor = vec4(1.0,0.0,0.0, 1.0); // set all 4 vector values to 1.0
}
