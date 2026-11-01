#version 330 core
out vec4 fragColor;
in vec2 texCoord;
uniform sampler2D displayTexture;
uniform sampler2D referenceTexture;
uniform bool comparison;
uniform bool showAlpha;
uniform bool showDifference;
uniform bool showBlocks;
uniform float blockSize;
uniform float zoom;
uniform vec2 viewCenter;

void main()
{
    vec2 uv = viewCenter + (texCoord - 0.5) / zoom;
    // DDS and ASTC assets preserve source image row order (top to bottom).
    // Apply the orientation correction in UVs, consistently for every format.
    vec2 imageUv = vec2(uv.x, 1.0 - uv.y);
    vec4 pixel = texture(displayTexture, imageUv);
    vec4 reference = texture(referenceTexture, imageUv);
    vec3 color;
    if (comparison && showDifference)
        color = showAlpha ? vec3(abs(pixel.a - reference.a) * 16.0)
                          : abs(pixel.rgb - reference.rgb) * 8.0;
    else if (showAlpha)
        color = vec3(pixel.a);
    else
    {
        float checker = mod(floor(texCoord.x * 24.0) + floor(texCoord.y * 24.0), 2.0);
        vec3 background = mix(vec3(0.12), vec3(0.24), checker);
        color = mix(background, pixel.rgb, pixel.a);
    }
    if (showBlocks)
    {
        vec2 cell = imageUv * 1024.0 / blockSize;
        vec2 edge = min(fract(cell), 1.0 - fract(cell));
        float border = 1.0 - smoothstep(0.0, 0.06, min(edge.x, edge.y));
        color = mix(color, vec3(1.0, 0.5, 0.1), border * 0.65);
    }
    fragColor = vec4(color, 1.0);
}
