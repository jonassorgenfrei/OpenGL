#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Vertex position in model space */

out vec3 fModelPosition;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ModelMatrix;             /**< Transforms from model space to world space */
layout (location = 1) uniform mat4 ViewMatrix;              /**< Transforms from world space to view space */
layout (location = 2) uniform mat4 ProjectionMatrix;        /**< Transforms from view space to clip space*/

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fModelPosition = vPosition.xyz;
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}

