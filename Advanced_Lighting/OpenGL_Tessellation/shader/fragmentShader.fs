#version 430 core

out vec4 FragColor;


in TES_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
} fs_in;

 uniform sampler2D diffuseMap;


void main() 
{
	
	FragColor = texture(diffuseMap, fs_in.texCoords);
	//FragColor = vec4(fs_in.texCoords, vec2(1.0));
}