#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D displayTexture;
uniform sampler2D physicalCache;
uniform sampler2D pageTable;
uniform bool virtualMode;
uniform vec2 viewCenter;
uniform float viewSpan;

const float virtualPages = 32.0;
const float pageSize = 64.0;
const float slotSize = 66.0;
const float cacheSize = 660.0;

void main()
{
    if (!virtualMode)
    {
        fragColor = texture(displayTexture, texCoord);
        return;
    }

    // Resolve the screen coordinate into logical texture and page space.
    vec2 virtualUv = viewCenter + (texCoord - 0.5) * viewSpan;
    vec2 pagePosition = virtualUv * virtualPages;
    ivec2 page = clamp(ivec2(floor(pagePosition)), ivec2(0), ivec2(31));
    vec4 entry = texelFetch(pageTable, page, 0);
    // Logical-space derivatives keep filtering stable across discontinuous
    // physical cache addresses.
    vec2 gradientX = dFdx(virtualUv) * (virtualPages * pageSize / cacheSize);
    vec2 gradientY = dFdy(virtualUv) * (virtualPages * pageSize / cacheSize);

    // Show missing residency while the bounded uploader fills the cache.
    if (entry.b < 0.5)
    {
        vec2 checkerCell = floor(pagePosition * 4.0);
        float checker = mod(checkerCell.x + checkerCell.y, 2.0);
        fragColor = vec4(mix(vec3(0.035, 0.04, 0.055), vec3(0.14, 0.045, 0.13), checker), 1.0);
        return;
    }

    // Remap the texel into its physical slot. Sampling inside the one-texel
    // gutter prevents linear filtering from leaking neighboring cache slots.
    vec2 slot = floor(entry.rg * 255.0 + 0.5);
    vec2 localUv = fract(pagePosition);
    vec2 cachePixel = slot * slotSize + vec2(1.5) + localUv * (pageSize - 1.0);
    vec2 cacheUv = cachePixel / cacheSize;
    vec3 color = textureGrad(physicalCache, cacheUv, gradientX, gradientY).rgb;

    // A subtle border makes page granularity visible for teaching and debugging.
    float edgeDistance = min(min(localUv.x, localUv.y), min(1.0 - localUv.x, 1.0 - localUv.y));
    float pageBorder = 1.0 - smoothstep(0.0, 0.018, edgeDistance);
    color = mix(color, vec3(0.02), pageBorder * 0.4);
    fragColor = vec4(color, 1.0);
}
