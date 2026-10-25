#version 430 core


// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec2 fVelocity;
in vec2 fPosition;

out vec4 FragColor;                             /**< Fragment color */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (binding = 0) uniform sampler1D ColorLookup;

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Computes a pseudo-random value from a 2D vector
 *
 * @param xi                    2D vector used as the random seed
 *
 * @return Pseudo-random value
 */
float rand(vec2 xi){
    return fract(sin(dot(xi.xy, vec2(12.9898,78.233))) * 43758.5453);
}

/**
 * Entry point for the fragment shader
 */
void main() {
    float texCoord = smoothstep(0, 200, length(fVelocity));
    vec4 color = texture(ColorLookup, rand(fVelocity)*pow(texCoord, 2));

    float a = 1 - length(2*gl_PointCoord - 1);

    color.a *= pow(a, 3);
    FragColor = color;
}