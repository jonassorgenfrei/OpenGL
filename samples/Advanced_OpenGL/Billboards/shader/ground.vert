#version 330   
// ----------------------------------------------------------------------------
//
// Outputs
//
// ----------------------------------------------------------------------------

out vec2 fTexCoord;

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    vec4 worldPosition = model*vec4(position, 1.0);
    gl_Position = projection*view*worldPosition;
    fTexCoord = texCoord;
}
