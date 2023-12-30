#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

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
    FragColor = Color;
}