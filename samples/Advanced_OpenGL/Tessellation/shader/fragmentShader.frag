#version 430 core

out vec4 FragColor;


in TES_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
} fs_in;

uniform sampler2D diffuseMap;
uniform int displayMode;


void main() 
{
	if (displayMode == 0) {
		FragColor = texture(diffuseMap, fs_in.texCoords);
	}
	else {
		FragColor = vec4(fs_in.texCoords, 0.0, 1.0);
	}
}
