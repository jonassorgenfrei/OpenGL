#version 330 core
in vec2 texCoord;
out vec4 fragColor;
uniform sampler2D atlas;
uniform sampler2DArray tileArray;
uniform bool useArray;
uniform bool storageView;
uniform bool inset;
uniform float lod;
uniform float repetitions;

void main()
{
    // Both panels use the same tile IDs and local coordinates. A real mesh
    // would pass an atlas rectangle or a flat array-layer index per material.
    vec2 grid = texCoord * 2.0;
    ivec2 cell = min(ivec2(floor(grid)), ivec2(1));
    int tile = (cell.x + cell.y * 2) % 4;
    vec2 localUv = fract(grid) * (storageView ? 1.0 : repetitions);
    vec3 color;
    if (storageView && !useArray)
        color = textureLod(atlas, texCoord, lod).rgb;
    else if (useArray)
        // S/T repeat and mip generation operate within each layer; filtering
        // never blends neighboring layers, even though all share one binding.
        color = textureLod(tileArray, vec3(localUv, float(tile)), lod).rgb;
    else
    {
        localUv = fract(localUv);
        vec2 origin = vec2(tile % 2, tile / 2);
        // A half-texel inset fixes base-level bilinear bleed only. Generating
        // mipmaps for the WHOLE atlas still mixes tiles at coarse levels.
        vec2 tileSize = vec2(textureSize(atlas, 0)) / 2.0;
        if (inset) localUv = mix(0.5 / tileSize, 1.0 - 0.5 / tileSize, localUv);
        vec2 atlasUv = (origin + localUv) / 2.0;
        color = textureLod(atlas, atlasUv, lod).rgb;
    }
    // Thin gaps separate the displayed quads, not the textures in storage.
    vec2 edge = min(fract(grid), 1.0 - fract(grid));
    if ((!storageView || useArray) && min(edge.x, edge.y) < 0.008) color = vec3(0.025);
    fragColor = vec4(color, 1.0);
}
