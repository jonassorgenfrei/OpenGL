#version 430 core


// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

in vec2 fVelocity;
in vec2 fPosition;

out vec4 FragColor;                             /**< Farbe des Fragments */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (binding = 0) uniform sampler1D ColorLookup;

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Berechnet auf Basis eines 2D-Vektors einen Zufallswert
 *
 * @param xi                    2D-Vektor als Grundlage des Zufallswertes
 *
 * @return Zufallswert
 */
float rand(vec2 xi){
    return fract(sin(dot(xi.xy, vec2(12.9898,78.233))) * 43758.5453);
}

/**
 * Einsprungpunkt für den Fragment-Shader
 */
void main() {
    float texCoord = smoothstep(0, 200, length(fVelocity));
    vec4 color = texture(ColorLookup, rand(fVelocity)*pow(texCoord, 2));

    float a = 1 - length(2*gl_PointCoord - 1);

    color.a *= pow(a, 3);
    FragColor = color;
}