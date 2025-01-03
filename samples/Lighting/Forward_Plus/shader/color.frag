
#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec4 fColor;         

out vec4 FragColor;   

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform vec4 Color; 

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

void main() {
    FragColor = fColor;
}
