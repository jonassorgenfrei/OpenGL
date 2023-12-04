#version 430 core
 
// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 ModelMatrix;               
uniform mat4 ViewMatrix;               
uniform mat4 ProjectionMatrix;          

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

void main() {
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}
