#version 430 core
// Composite a peeled transparent layer into the accumulation buffer (premultiplied).


// Composite a single peeled transparent layer into the accumulation buffer.
// Input colors are premultiplied; alpha carries remaining visibility.

// shader outputs
out vec4 FragColor;

// Uniforms
uniform sampler2D peelTex;     // peeled transparent layer
uniform ivec2 screenSize;      // width, height of framebuffer

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(screenSize);
    vec4 peelColor = texture(peelTex, uv);
    FragColor = peelColor;
}
