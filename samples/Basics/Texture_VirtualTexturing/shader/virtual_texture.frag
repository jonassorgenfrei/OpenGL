#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D physicalCache;
uniform sampler2D pageTable;
uniform vec2 viewCenter;
uniform float viewSpan;
uniform int activeMip;

const float pageSize = 64.0;
const float slotSize = 66.0;
const float cacheSize = 726.0;

int pageDimension(int mip) { return 32 >> mip; }

int pageTableOffset(int mip)
{
    // Vertical layout: 32 + 16 + 8 + 4 + 2 + 1 page rows.
    if (mip == 0) return 0;
    if (mip == 1) return 32;
    if (mip == 2) return 48;
    if (mip == 3) return 56;
    if (mip == 4) return 60;
    return 62;
}

void main()
{
    vec2 virtualUv = viewCenter + (texCoord - 0.5) * viewSpan;
    vec4 entry = vec4(0.0);
    int sampledMip = activeMip;

    // Walk toward the permanently resident coarse levels until a mapping is
    // found. Fine pages replace this fallback as the bounded uploader runs.
    for (int mip = 0; mip < 6; ++mip)
    {
        if (mip < activeMip) continue;
        int dimension = pageDimension(mip);
        ivec2 page = clamp(ivec2(floor(virtualUv * float(dimension))),
                           ivec2(0), ivec2(dimension - 1));
        entry = texelFetch(pageTable,
                           ivec2(page.x, page.y + pageTableOffset(mip)), 0);
        sampledMip = mip;
        if (entry.b >= 0.5) break;
    }

    float sampledPages = float(pageDimension(sampledMip));
    vec2 pagePosition = virtualUv * sampledPages;
    if (entry.b < 0.5)
    {
        // Defensive diagnostic: pinned fallback pages make this unreachable
        // during normal operation.
        vec2 cell = floor(pagePosition * 4.0);
        float checker = mod(cell.x + cell.y, 2.0);
        fragColor = vec4(mix(vec3(0.035, 0.04, 0.055),
                             vec3(0.14, 0.045, 0.13), checker), 1.0);
        return;
    }

    vec2 slot = floor(entry.rg * 255.0 + 0.5);
    vec2 localUv = fract(pagePosition);
    vec2 cachePixel = slot * slotSize + vec2(1.5) + localUv * (pageSize - 1.0);
    vec2 cacheUv = cachePixel / cacheSize;
    vec2 gradientX = dFdx(virtualUv) * (sampledPages * pageSize / cacheSize);
    vec2 gradientY = dFdy(virtualUv) * (sampledPages * pageSize / cacheSize);
    vec3 color = textureGrad(physicalCache, cacheUv, gradientX, gradientY).rgb;

    // The subtle border exposes page granularity for teaching and debugging.
    float edge = min(min(localUv.x, localUv.y),
                     min(1.0 - localUv.x, 1.0 - localUv.y));
    float border = 1.0 - smoothstep(0.0, 0.018, edge);
    fragColor = vec4(mix(color, vec3(0.02), border * 0.4), 1.0);
}
