#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D displayTexture;
uniform sampler2D physicalCache;
uniform sampler2D pageTable;
uniform bool virtualMode;
uniform vec2 viewCenter;
uniform float viewSpan;
uniform int activeMip;

const float pageSize = 64.0;
const float slotSize = 66.0;
const float cacheSize = 726.0;

int pageDimension(int mip)
{
    return 32 >> mip;
}

int pageTableOffset(int mip)
{
    // The mip page tables are packed vertically: 32 + 16 + 8 + 4 + 2 + 1.
    if (mip == 0) return 0;
    if (mip == 1) return 32;
    if (mip == 2) return 48;
    if (mip == 3) return 56;
    if (mip == 4) return 60;
    return 62;
}

void main()
{
    if (!virtualMode)
    {
        fragColor = texture(displayTexture, texCoord);
        return;
    }

    // Resolve the screen coordinate into logical texture space, then select
    // the requested mip or the first coarser page that is already resident.
    vec2 virtualUv = viewCenter + (texCoord - 0.5) * viewSpan;
    vec4 entry = vec4(0.0);
    int sampledMip = activeMip;
    for (int mip = 0; mip < 6; ++mip)
    {
        if (mip < activeMip) continue;
        int dimension = pageDimension(mip);
        vec2 candidatePosition = virtualUv * float(dimension);
        ivec2 page = clamp(ivec2(floor(candidatePosition)), ivec2(0), ivec2(dimension - 1));
        entry = texelFetch(pageTable, ivec2(page.x, page.y + pageTableOffset(mip)), 0);
        sampledMip = mip;
        if (entry.b >= 0.5) break;
    }

    float sampledPages = float(pageDimension(sampledMip));
    vec2 pagePosition = virtualUv * sampledPages;
    // Logical-space derivatives keep filtering stable across discontinuous
    // physical cache addresses.
    vec2 gradientX = dFdx(virtualUv) * (sampledPages * pageSize / cacheSize);
    vec2 gradientY = dFdy(virtualUv) * (sampledPages * pageSize / cacheSize);

    // The two coarsest levels are pinned, so this is only a defensive marker
    // for a broken page-table mapping rather than normal streaming behavior.
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
