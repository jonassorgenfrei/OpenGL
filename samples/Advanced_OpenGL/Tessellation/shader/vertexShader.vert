#version 430 core
/**
 * Pass model-space attributes to the tessellation stages. Projection is
 * deferred until the TES because tessellation operates on object-space patches.
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
	// Keep the position in model space; the TES performs the final projection.
	vs_out.positionW = (model * vec4(aPos,1.0)).xyz;
	vs_out.texCoords = aTexCoords;
	vs_out.normal = (model * vec4(aNormal,1.0)).xyz;

	// The tessellation evaluation shader writes the final gl_Position value.
}
