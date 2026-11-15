#version 330 core

out vec4 FragColor;

in VS_OUT {
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform bool normalMappingEnabled;
uniform bool showNormals;

vec3 blinnPhong(vec3 normal, vec3 color)
{
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float diffuseStrength = max(dot(lightDir, normal), 0.0);
	float specularStrength = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	return diffuseStrength * color + specularStrength * vec3(0.2);
}

void main()
{
	vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
	vec3 mappedNormal = normalize(texture(normalMap, fs_in.TexCoords).rgb * 2.0 - 1.0);
	vec3 normal = normalMappingEnabled ? mappedNormal : vec3(0.0, 0.0, 1.0);

	if (showNormals) {
		FragColor = vec4(normal * 0.5 + 0.5, 1.0);
		return;
	}

	vec3 ambient = 0.1 * color;
	FragColor = vec4(ambient + blinnPhong(normal, color), 1.0);
}
