#version 330   

// ----------------------------------------------------------------------------
//
// Attributess
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec4 gPosition;                         /**< Vertex position in world space */
out vec3 gNormal;                           /**< Vertex normal in world space */

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    gPosition = vec4(position, 1);
    gNormal = normal;
}
