#version 330 core

out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;

/*
 * Normal vectors in a normal map are expressed in tangent space and point
 * roughly in the positive z direction.
 */
uniform sampler2D normalMap;
uniform sampler2D depthMap;

uniform float height_scale;
uniform bool parallaxEnabled;
uniform bool normalMappingEnabled;
uniform bool showNormals;
uniform int parallaxMethod;

/*
 * ParallaxMapping
 *  @param texCoords - fragment texture coordinates
 *  @param viewDir   - fragment-to-view direction in tangent space
 *  @return displaced texture coordinates
 *
 * The keyboard selects the implementation at runtime:
 *  1 = single-sample parallax mapping
 *  2 = steep parallax mapping
 *  3 = parallax occlusion mapping
 */
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	/*
	 * Parallax Mapping (one sample)
	 *
	 * Sample the depth at the original coordinate and offset the coordinate
	 * along the projected view direction. Dividing by viewDir.z increases the
	 * displacement at grazing angles. Clamping z avoids an unstable division
	 * when the view direction is almost parallel to the surface.
	 */
	if (parallaxMethod == 1) {
		float depth = texture(depthMap, texCoords).r;
		vec2 p = viewDir.xy / max(viewDir.z, 0.05) * depth * height_scale;
		return texCoords - p;
	}

	/*
	 * Steep Parallax Mapping
	 *
	 * Divide the complete depth range into layers and shift the texture
	 * coordinate by a small step for every layer. Stop at the first point at
	 * which the sampled depth lies above the current layer depth. More layers
	 * reduce visible stepping, but require more texture samples.
	 */
	const float minLayers = 8.0;
	const float maxLayers = 32.0;

	// Looking straight at the surface needs fewer samples than a grazing angle.
	float numLayers = mix(maxLayers, minLayers, abs(viewDir.z));
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;

	vec2 P = viewDir.xy / max(viewDir.z, 0.05) * height_scale;
	vec2 deltaTexCoords = P / numLayers;
	vec2 currentTexCoords = texCoords;
	float currentDepthMapValue = texture(depthMap, currentTexCoords).r;

	while (currentLayerDepth < currentDepthMapValue) {
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = texture(depthMap, currentTexCoords).r;
		currentLayerDepth += layerDepth;
	}

	// Steep parallax uses the first sample beyond the intersection.
	if (parallaxMethod == 2)
		return currentTexCoords;

	/*
	 * Parallax Occlusion Mapping
	 *
	 * Steep parallax only finds an interval containing the intersection.
	 * Interpolate between the samples before and after the collision for a
	 * smoother result without the cost of a full relief-mapping refinement.
	 */
	vec2 previousTexCoords = currentTexCoords + deltaTexCoords;
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(depthMap, previousTexCoords).r
		- currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);

	return mix(currentTexCoords, previousTexCoords, weight);
}

/*
 * Blinn-Phong lighting model. The normal and every position passed to this
 * function are expressed in tangent space.
 */
vec3 BlinnPhong(
	vec3 normal,
	vec3 fragPosition,
	vec3 lightPosition,
	vec3 viewPosition,
	vec3 lightColor)
{
	// Diffuse reflection
	vec3 lightDir = normalize(lightPosition - fragPosition);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;

	// Specular reflection
	vec3 viewDir = normalize(viewPosition - fragPosition);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	vec3 specular = spec * vec3(0.2);

	return diffuse + specular;
}

/*
 * There are two common ways to use the TBN matrix:
 *  1) transform the sampled normal from tangent space to world space;
 *  2) transform light, view, and fragment positions to tangent space in the
	 *     vertex shader. This sample uses the second approach.
 */
void main()
{
	// Offset the texture coordinates using the selected parallax implementation.
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec2 texCoords = parallaxEnabled
		? ParallaxMapping(fs_in.TexCoords, viewDir)
		: fs_in.TexCoords;

	/*
	 * Discard fragments whose displaced coordinates leave the texture. This
	 * works for the demonstration quad, but is not suitable for every surface.
	 */
	if (any(greaterThan(texCoords, vec2(1.0))) ||
		any(lessThan(texCoords, vec2(0.0))))
		discard;

	vec3 color = texture(diffuseMap, texCoords).rgb;

	// Decode the sampled normal from [0, 1] to tangent-space range [-1, 1].
	vec3 mappedNormal = texture(normalMap, texCoords).rgb;
	mappedNormal = normalize(mappedNormal * 2.0 - 1.0);

	// The geometric normal is +z in tangent space.
	vec3 normal = normalMappingEnabled ? mappedNormal : vec3(0.0, 0.0, 1.0);

	if (showNormals) {
		FragColor = vec4(normal * 0.5 + 0.5, 1.0);
		return;
	}

	vec3 ambient = 0.1 * color;
	vec3 lighting = BlinnPhong(
		normal,
		fs_in.TangentFragPos,
		fs_in.TangentLightPos,
		fs_in.TangentViewPos,
		color);

	FragColor = vec4(ambient + lighting, 1.0);
}
