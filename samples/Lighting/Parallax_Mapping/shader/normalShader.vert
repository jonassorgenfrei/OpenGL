#version 330 core
/**
 * Using tangent space, to to lightning in a different coordinate space.
 * A coordinate space, where the normal map vectors always point roughly in the positive z direction.
 *
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

out mat3 tbn;

void main() {
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	vs_out.TexCoords = aTexCoords;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	/* 
	 * Tangent Space 
	 *	space that's local to the surface of a triangle
	 *	TBN-Matrix: Tangent, Bitangent, Normal
	 */
	// no normal matrix used, vectors might get scaling and/or translation
	vec3 T = normalize(vec3(model*vec4(aTangent, 0.0)));		// transform to coord. System to work in (WORLD SPACE)
	vec3 N = normalize(vec3(model*vec4(aNormal, 0.0)));			// transform to coord. System to work in (WORLD SPACE)

	/* Larger Mesh: tangent vect. are generally averaged (smooth result) 
	 *		Problem: maybe not perpendicular to each other => matrix wouldn'T be orthogonal anymore
	 *		Handeling: Using Gram-Schmidt process (re-orthogonalize) 
	 */
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N)*N);
	vec3 B = cross(N,T);
	
	// vec3 B = normalize(vec3(model*vec4(aBitangent, 0.0)));			// transform to coord. System to work in (WORLD SPACE)

	/* TBN must form a right handed orrd system
	 * Some models have symetric UVs. Check and fixed
	 */
	if (dot(cross(N, T), B) < 0.0) {
		T = T * -1.0;
	}

	//vec3 B = normalize(vec3(model*vec4(aBitangent, 0.0)));		// transform to coord. System to work in (WORLD SPACE) 
	mat3 TBN = transpose(mat3(T,B,N)); // same like inverse, cause orthogonal matrix: transpose == inverse. Calc transpose is faster
	tbn = TBN;

	vs_out.TangentLightPos = TBN * lightPos; // for parallax mapping important to be in tangent space
	vs_out.TangentViewPos = TBN * viewPos; // for parallax mapping important to be in tangent space
	vs_out.TangentFragPos = TBN * vs_out.FragPos;

	gl_Position = projection * view * model * vec4(aPos, 1.0);
}