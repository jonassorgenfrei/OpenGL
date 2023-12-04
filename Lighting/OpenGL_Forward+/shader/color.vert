#version 430 core
 
// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition; 
layout (location = 3) in vec4 vColor;

out vec4 fColor;

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
    fColor = vColor;
    gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix* vPosition;
}
