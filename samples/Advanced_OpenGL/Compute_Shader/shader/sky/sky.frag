#version 430 core
// ----------------------------------------------------------------------------
//
// Konstanten
//
// ----------------------------------------------------------------------------

const float PI  = 3.14159265358979;

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec3 fModelPosition;

layout (location = 0) out vec4 FragColor;                             /**< Fragment color */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 3) uniform vec4 Color;       /**< Fragment color */

layout (binding = 0) uniform sampler2D EnvironmentMap;

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

vec4 sampleEquirectangular(vec3 dir, sampler2D sampler, float level)  {
        vec2 uv;
        uv.x = atan(dir.z, dir.x);
        uv.y = acos(dir.y);
        uv /= vec2(2 * PI, PI);

        return textureLod(sampler, uv, level);
}


/**
 * Entry point for the fragment shader
 */
void main() {
    FragColor = sampleEquirectangular(normalize(fModelPosition), EnvironmentMap, 0);
}
