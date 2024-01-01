#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Position des Partikels */
layout (location = 1) in vec2 vVelocity;                    /**< Geschwindigkeit des Partikels */

out vec2 fVelocity;
out vec2 fPosition;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ProjectionMatrix;        /**< Transformation vom Kamera- ins Clipping-Koordinatensystem*/

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fVelocity = vVelocity;
    fPosition = vPosition.xy;
    gl_Position = ProjectionMatrix*vPosition;
}
