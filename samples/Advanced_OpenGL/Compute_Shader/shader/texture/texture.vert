#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;    /**< Vertex position in model space */
layout (location = 2) in vec2 vTexCoord;    /**< Vertex texture coordinate */

out vec2 fTexCoord;

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fTexCoord = vTexCoord;
    gl_Position = vPosition;
}
