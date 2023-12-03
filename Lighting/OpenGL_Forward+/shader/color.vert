#version 430 core
 
// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;  /**< Position des Vertexes im Modell-Koordinatensystem */
layout (location = 3) in vec4 vColor;     /**< Position des Vertexes im Modell-Koordinatensystem */

out vec4 fColor;                          /**< Farbe des Vertexes für den Fragment Shader */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 ModelMatrix;                 /**< Transformation vom Modell- ins Welt-Koordinatensystem */
uniform mat4 ViewMatrix;                  /**< Transformation vom Welt- ins Kamera-Koordinatensystem */
uniform mat4 ProjectionMatrix;            /**< Transformation vom Kamera- ins Clipping-Koordinatensystem*/

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fColor = vColor;
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}
