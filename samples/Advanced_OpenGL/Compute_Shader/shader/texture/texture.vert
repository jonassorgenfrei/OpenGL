#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;    /**< Position des Vertexes im Modell-Koordinatensystem */
layout (location = 2) in vec2 vTexCoord;    /**< Textur-Koordinate des Vertexes */

out vec2 fTexCoord;

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fTexCoord = vTexCoord;
    gl_Position = vPosition;
}
