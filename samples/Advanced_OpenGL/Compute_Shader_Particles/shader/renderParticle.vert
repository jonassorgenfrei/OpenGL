#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Particle position */
layout (location = 1) in vec2 vVelocity;                    /**< Particle velocity */

out vec2 fVelocity;
out vec2 fPosition;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ProjectionMatrix;        /**< Transforms from view space to clip space*/

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fVelocity = vVelocity;
    fPosition = vPosition.xy;
    gl_Position = ProjectionMatrix*vPosition;
}
