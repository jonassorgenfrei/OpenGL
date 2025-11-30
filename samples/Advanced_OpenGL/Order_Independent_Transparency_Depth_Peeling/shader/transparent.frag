#version 430 core

// Depth peeling transparent fragment shader.
// Pass 0 peels the nearest transparent layer in front of the opaque depth.
// Subsequent passes peel progressively deeper layers by comparing against the previous peel depth.

// shader outputs
out vec4 FragColor;

// shader inputs
in vec4 fragPos; // clip-space position

// uniforms
uniform sampler2D prevDepthTex;
uniform sampler2D opaqueDepthTex;
uniform int pass;

// material color
uniform vec4 color;

void main()
{
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float prevDepth = texelFetch(prevDepthTex, coords, 0).r;
    float opaqueDepth = texelFetch(opaqueDepthTex, coords, 0).r;

    // stop peeling if previous layer had no hits at this pixel
    if (pass > 0 && prevDepth >= 1.0)
        discard;

    // drop fragments behind opaque
    if (gl_FragCoord.z >= opaqueDepth)
        discard;

    // for passes >0, also drop fragments that are not deeper than the last peel (when valid)
    if (pass > 0 && prevDepth < 1.0 && gl_FragCoord.z <= prevDepth)
        discard;

    // store premultiplied color so front-to-back blending accumulates correctly
    FragColor = vec4(color.rgb * color.a, color.a);
}
