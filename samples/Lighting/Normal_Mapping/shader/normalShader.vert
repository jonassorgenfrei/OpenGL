#version 330 core
/**
 * Perform lighting in tangent space, where decoded normal-map vectors point
 * roughly along the positive z-axis.
 */

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	vs_out.TexCoords = aTexCoords;

	/*
	 * Tangent space is local to a triangle's surface. Its basis is formed by
	 * tangent (T), bitangent (B), and geometric normal (N).
	 */
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 N = normalize(normalMatrix * aNormal);
	vec3 T = normalize(mat3(model) * aTangent);

	/*
	 * On larger meshes tangent vectors are generally averaged for a smooth
	 * result. They may no longer be perpendicular to N, so Gram-Schmidt
	 * re-orthogonalization restores an orthonormal basis.
	 */
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	/*
	 * Mirrored UVs can change the handedness of tangent space. Compare the
	 * reconstructed bitangent with the bitangent supplied by the mesh.
	 */
	vec3 meshBitangent = normalize(mat3(model) * aBitangent);
	B *= dot(B, meshBitangent) < 0.0 ? -1.0 : 1.0;

	/*
	 * mat3(T, B, N) transforms tangent-space vectors to world space. Because
	 * the basis is orthonormal, its transpose is its inverse and transforms
	 * world-space vectors to tangent space.
	 */
	mat3 worldToTangent = transpose(mat3(T, B, N));
	vs_out.TangentLightPos = worldToTangent * lightPos;
	vs_out.TangentViewPos = worldToTangent * viewPos;
	vs_out.TangentFragPos = worldToTangent * vs_out.FragPos;

	gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
