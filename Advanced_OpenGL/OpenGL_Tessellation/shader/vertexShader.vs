#version 430 core
/**
 * Using tangent space, to to lightning in a different coordinate space.
 * A coordinate space, where the normal map vectors always point roughly in the positive z direction.
 *
 */

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;

out VS_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
} vs_out;

void main() {
	// Don't transform local space coordinates to clip space (view-projection-Matrix)
	// postphone this Action
	vs_out.positionW = (model * vec4(aPos,1.0)).xyz;
	vs_out.texCoords = aTexCoords;
	vs_out.normal = (model * vec4(aNormal,1.0)).xyz;

	// Die vordefinierte Variable gl_Position wird im Tessellation 
	// Evaluation Shader beschrieben.
}