#version 330 core
out vec4 FragColor;

struct LightColor {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 Color;
in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform LightColor lightColor;


void main()
{
	vec3 result;
	result = (lightColor.ambient + lightColor.diffuse + lightColor.specular)/3;

	FragColor = vec4(result, 1.0); // set all 4 vector values to 1.0
}
