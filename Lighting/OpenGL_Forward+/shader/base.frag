#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

struct Material {
	//Textures
	sampler2D texture_height1;
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
	sampler2D texture_emission1;
    float shininess;
}; 

//Materials
uniform Material material;

void main() {
	vec4 diffuse = texture(material.texture_diffuse1, TexCoords);

	// alpha discard
	if(diffuse.w < 0.5)
		discard;

	FragColor = vec4(vec3(diffuse), 1.0);
}