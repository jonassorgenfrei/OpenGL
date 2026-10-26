#version 330 core
uniform sampler2D positions;
uniform float currentFrame;
in vec2 uv;
out vec4 FragColor;
void main()
{
    ivec2 size = textureSize(positions, 0);
    ivec2 texel = clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1);
    vec3 position = texelFetch(positions, texel, 0).rgb;
    // Remap signed XYZ only for display. The VAT itself keeps raw positions.
    vec3 color = position / vec3(4.0, 2.4, 1.2) + 0.5;
    if (texel.y == int(floor(currentFrame)))
        color = vec3(1.0);
    FragColor = vec4(color, 1.0);
}
