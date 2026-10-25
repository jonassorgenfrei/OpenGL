#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Vertex position in model space */

out vec4 fDcPosition;                                       /**< Vertex position in clip space */

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fDcPosition = vPosition;
    gl_Position = vPosition;
}
