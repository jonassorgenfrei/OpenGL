#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Position des Vertex im Modell-Koordinatensystem */

out vec4 fDcPosition;                                       /**< Position des Vertex im Clipping Koordinatensystem */

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fDcPosition = vPosition;
    gl_Position = vPosition;
}
