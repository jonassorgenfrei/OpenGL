#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Position des Vertexes im Modell-Koordinatensystem */

out vec3 fModelPosition;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ModelMatrix;             /**< Transformation vom Modell- ins Welt-Koordinatensystem */
layout (location = 1) uniform mat4 ViewMatrix;              /**< Transformation vom Welt- ins Kamera-Koordinatensystem */
layout (location = 2) uniform mat4 ProjectionMatrix;        /**< Transformation vom Kamera- ins Clipping-Koordinatensystem*/

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fModelPosition = vPosition.xyz;
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}

