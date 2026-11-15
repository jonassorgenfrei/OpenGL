#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
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
	vec3 fragPos = vec3(model * vec4(aPos, 1.0));
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 N = normalize(normalMatrix * aNormal);
	vec3 T = normalize(mat3(model) * aTangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	// Preserve the handedness encoded by the mesh's bitangent.
	vec3 meshBitangent = normalize(mat3(model) * aBitangent);
	B *= dot(B, meshBitangent) < 0.0 ? -1.0 : 1.0;

	// The transpose converts world-space vectors into tangent space.
	mat3 worldToTangent = transpose(mat3(T, B, N));
	vs_out.TexCoords = aTexCoords;
	vs_out.TangentLightPos = worldToTangent * lightPos;
	vs_out.TangentViewPos = worldToTangent * viewPos;
	vs_out.TangentFragPos = worldToTangent * fragPos;

	gl_Position = projection * view * vec4(fragPos, 1.0);
}
