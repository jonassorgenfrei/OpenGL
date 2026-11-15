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
uniform sampler2D depthMap;
uniform float height_scale;
uniform bool parallaxEnabled;
uniform bool normalMappingEnabled;
uniform bool showNormals;
uniform int parallaxMethod;

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
	if (parallaxMethod == 1) {
		float depth = texture(depthMap, texCoords).r;
		return texCoords - viewDir.xy / max(viewDir.z, 0.05) * depth * height_scale;
	}

	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, abs(viewDir.z));
	float layerDepth = 1.0 / numLayers;
	vec2 deltaTexCoords = viewDir.xy / max(viewDir.z, 0.05) * height_scale / numLayers;

	vec2 currentTexCoords = texCoords;
	float currentLayerDepth = 0.0;
	float currentMapDepth = texture(depthMap, currentTexCoords).r;
	while (currentLayerDepth < currentMapDepth) {
		currentTexCoords -= deltaTexCoords;
		currentMapDepth = texture(depthMap, currentTexCoords).r;
		currentLayerDepth += layerDepth;
	}

	// Method 2 stops at the first depth-layer intersection (steep parallax).
	if (parallaxMethod == 2)
		return currentTexCoords;

	// Method 3 interpolates between the last two samples (parallax occlusion).
	vec2 previousTexCoords = currentTexCoords + deltaTexCoords;
	float afterDepth = currentMapDepth - currentLayerDepth;
	float beforeDepth = texture(depthMap, previousTexCoords).r
		- currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	return mix(currentTexCoords, previousTexCoords, weight);
}

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
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec2 texCoords = parallaxEnabled
		? parallaxMapping(fs_in.TexCoords, viewDir)
		: fs_in.TexCoords;

	if (any(greaterThan(texCoords, vec2(1.0))) || any(lessThan(texCoords, vec2(0.0))))
		discard;

	vec3 color = texture(diffuseMap, texCoords).rgb;
	vec3 mappedNormal = normalize(texture(normalMap, texCoords).rgb * 2.0 - 1.0);
	vec3 normal = normalMappingEnabled ? mappedNormal : vec3(0.0, 0.0, 1.0);

	if (showNormals) {
		FragColor = vec4(normal * 0.5 + 0.5, 1.0);
		return;
	}

	vec3 ambient = 0.1 * color;
	FragColor = vec4(ambient + blinnPhong(normal, color), 1.0);
}
