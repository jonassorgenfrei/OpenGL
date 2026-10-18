#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D physicalCache;
uniform sampler2D pageTable;
uniform vec2 viewCenter;
uniform float viewSpan;
uniform int activeMip;
uniform int virtualPages;
uniform int mipLevels;
uniform int hoveredSlot;
uniform int hoveredMip;
uniform vec4 hoveredBounds; // Logical page bounds (min UV, max UV), excluding gutters.
uniform int displayMode; // 0: image, 1: residency, 2: cache, 3: overview

const float pageSize = 64.0;
const float slotSize = 66.0;
const float cacheSize = 726.0;

int pageDimension(int mip) { return virtualPages >> mip; }

int pageTableOffset(int mip)
{
    int offset = 0;
    for (int level = 0; level < mip; ++level) offset += pageDimension(level);
    return offset;
}

void main()
{
    if (displayMode == 2)
    {
        vec3 color = texture(physicalCache, texCoord).rgb;
        vec2 cell = texCoord * (cacheSize / slotSize);
        vec2 cellWidth = max(fwidth(cell), vec2(1e-6));
        int slotIndex = int(floor(cell.y)) * int(cacheSize / slotSize) + int(floor(cell.x));
        if (slotIndex == hoveredSlot)
        {
            vec2 edgePixels = min(fract(cell), 1.0 - fract(cell)) / cellWidth;
            vec3 mark = hoveredMip >= 0 ? vec3(0.0, 1.0, 1.0) : vec3(0.6);
            color = min(edgePixels.x, edgePixels.y) < 2.0 ? mark : mix(color, mark, 0.15);
        }
        fragColor = vec4(color, 1.0);
        return;
    }
    vec2 virtualUv = viewCenter + (texCoord - 0.5) * viewSpan;
    if (displayMode == 3) virtualUv = texCoord;
    vec4 entry = vec4(0.0);
    int firstMip = displayMode == 3 ? mipLevels - 2 : activeMip;
    int sampledMip = firstMip;

    // Walk toward the permanently resident coarse levels until a mapping is
    // found. Fine pages replace this fallback as the bounded uploader runs.
    for (int mip = 0; mip < mipLevels; ++mip)
    {
        if (mip < firstMip) continue;
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
    vec2 cachePixel = slot * slotSize + vec2(1.0) + localUv * pageSize;
    vec2 cacheUv = cachePixel / cacheSize;
    vec2 gradientX = dFdx(virtualUv) * (sampledPages * pageSize / cacheSize);
    vec2 gradientY = dFdy(virtualUv) * (sampledPages * pageSize / cacheSize);
    vec3 color = textureGrad(physicalCache, cacheUv, gradientX, gradientY).rgb;

    if (displayMode == 3)
    {
        vec2 distanceToBox = abs(virtualUv - viewCenter) - vec2(viewSpan * 0.5);
        float edgeDistance = max(distanceToBox.x, distanceToBox.y);
        if (abs(edgeDistance) < 0.006) color = vec3(1.0, 0.85, 0.15);
        else if (edgeDistance > 0.0) color *= 0.5;
    }
    if (displayMode == 1)
        color = mix(color, sampledMip == activeMip ? vec3(0.1, 0.9, 0.35)
                                                       : vec3(1.0, 0.3, 0.08), 0.65);

    // The subtle border exposes page granularity for teaching and debugging.
    float edge = min(min(localUv.x, localUv.y),
                     min(1.0 - localUv.x, 1.0 - localUv.y));
    float border = 1.0 - smoothstep(0.0, 0.018, edge);
    color = mix(color, vec3(0.02), border * (displayMode == 1 ? 0.4 : 0.0));
    if (hoveredMip >= 0)
    {
        // Mark the logical footprint even if a different mip currently supplies
        // these texels. The overview also locates cached pages outside the view.
        vec2 center = (hoveredBounds.xy + hoveredBounds.zw) * 0.5;
        vec2 halfSize = (hoveredBounds.zw - hoveredBounds.xy) * 0.5;
        vec2 signedPixels = (abs(virtualUv - center) - halfSize) / max(fwidth(virtualUv), vec2(1e-8));
        float distanceToEdge = max(signedPixels.x, signedPixels.y);
        vec3 cyan = vec3(0.0, 1.0, 1.0);
        // Pixel-sized strokes keep fine pages visible even in the whole-image overview.
        if (distanceToEdge >= -2.0 && distanceToEdge <= 1.5) color = cyan;
        else if (distanceToEdge < -2.0) color = mix(color, cyan, 0.25);
    }
    fragColor = vec4(color, 1.0);
}
