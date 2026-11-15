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

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool normalMappingEnabled;
uniform bool showNormals;
uniform bool blinn;
uniform bool gammaEnabled;

/*
 * Phong / Blinn-Phong lighting model.
 * All positions passed to this function must be in the same coordinate space
 * as the normal. For this sample, that is tangent space.
 */
vec3 BlinnPhong(
	vec3 normal,
	vec3 fragPosition,
	vec3 lightPosition,
	vec3 viewPosition,
	vec3 lightColor,
	bool useBlinn)
{
	// Diffuse reflection
	vec3 lightDir = normalize(lightPosition - fragPosition);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;

	// Specular reflection: switch between Phong and Blinn-Phong at runtime.
	vec3 viewDir = normalize(viewPosition - fragPosition);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = useBlinn
		? pow(max(dot(normal, halfwayDir), 0.0), 32.0)
		: pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
	vec3 specular = spec * vec3(0.2);

	return diffuse + specular;
}

/*
 * There are two common ways to use the TBN matrix:
 *  1) pass TBN to the fragment shader and transform the sampled normal from
 *     tangent space to world space;
 *  2) use inverse(TBN) in the vertex shader to transform the light, view, and
 *     fragment positions from world space to tangent space.
 * The second approach is used here because the vertex shader runs less often.
 */
void main()
{
	// Diffuse textures contain display-space colors. Decode them before lighting.
	vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
	if (gammaEnabled)
		color = pow(color, vec3(2.2));

	vec3 ambient = 0.1 * color;

	// Decode the sampled normal from [0, 1] to the tangent-space range [-1, 1].
	vec3 mappedNormal = texture(normalMap, fs_in.TexCoords).rgb;
	mappedNormal = normalize(mappedNormal * 2.0 - 1.0);

	// The geometric normal is +z in tangent space.
	vec3 normal = normalMappingEnabled ? mappedNormal : vec3(0.0, 0.0, 1.0);

	if (showNormals) {
		FragColor = vec4(normal * 0.5 + 0.5, 1.0);
		return;
	}

	// Correct: normal, fragment, light, and view position are all in tangent space.
	vec3 lighting = BlinnPhong(
		normal,
		fs_in.TangentFragPos,
		fs_in.TangentLightPos,
		fs_in.TangentViewPos,
		color,
		blinn);

	/*
	 * Incorrect comparison: a tangent-space normal cannot be combined directly
	 * with world-space positions.
	 * vec3 lighting = BlinnPhong(
	 *     normal, fs_in.FragPos, lightPos, viewPos, color, blinn);
	 */

	vec3 result = ambient + lighting;
	if (gammaEnabled)
		result = pow(result, vec3(1.0 / 2.2));

	FragColor = vec4(result, 1.0);
}
