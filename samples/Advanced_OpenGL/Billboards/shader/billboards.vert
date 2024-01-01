#version 330   

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec4 gPosition;                         /**< Position des Vertexes im Welt-Koordinatensystem */
out vec3 gNormal;                           /**< Normale des Vertexes im Welt-Koordinatensystem */

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    gPosition = vec4(position, 1);
    gNormal = normal;
}
