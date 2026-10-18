# Texture atlas and texture array

Compare the same four generated material tiles side by side using an OpenGL
3.3 context. The left panel packs them into a **128 x 128 texture atlas**;
the right panel stores them in **four 64 x 64 texture-array layers**. Both
approaches select multiple materials from a single texture binding. Neither
approach by itself guarantees fewer draw calls; this demo draws one quad per
panel and selects tile IDs in the fragment shader.

| Property | Atlas (`GL_TEXTURE_2D`) | Array (`GL_TEXTURE_2D_ARRAY`) |
| --- | --- | --- |
| Upload | `glTexImage2D` | `glTexImage3D` + `glTexSubImage3D` per layer |
| GLSL lookup | `sampler2D`, scaled/offset UV | `sampler2DArray`, `vec3(uv, layer)` |
| Tile selection | Rectangle origin and size | Discrete layer index |
| Tile dimensions | Can pack different rectangles | All layers have equal size and format |
| Filtering | Can reach adjacent tiles | Never blends different layers |
| Repetition | Wrap local UV within the rectangle | S/T wrap within each layer |
| Mip generation | Whole-image generation can mix tiles | Each layer is filtered independently |
| Base RGBA8 payload in this demo | 64 KiB | 64 KiB |

Arrays are not 3D volume textures: the layer count stays fixed across mip levels,
and the layer coordinate does not interpolate between layers. Both approaches
have mipmap overhead; the displayed 64 KiB figures cover their base images only.

## Try this walkthrough

1. Compare the repeated material grids at the initial mip level 3. Colored seams
   appear inside the atlas tiles where linear filtering reaches other materials;
   array filtering stays inside each layer.
2. Press **Down** three times to select mip 0. Toggle **G**: a half-texel inset
   keeps base-level atlas sampling inside each tile's edge texels.
3. Increase the mip level with **Up**. The base-level inset no longer protects
   the larger filtering footprint. At mip 7 (the final 1 x 1 atlas level), generating
   mipmaps for the entire packed image also mixes unrelated tiles. Correct atlas
   pipelines need gutters, per-tile mip generation, and a suitable maximum LOD.
   This demo intentionally retains the naive atlas to expose the problem.
4. Press **T** to compare one copy with four repetitions per tile. The atlas
   wraps local coordinates manually; the array uses per-layer texture wrapping.
5. Press **M** to inspect storage: the left shows the actual packed atlas and
   the right displays the four independent layers as separate cards.

The shader uses explicit integer `textureLod` values (0 through 7; the array clamps to its last level, 6) so that both
panels can be compared at a controlled filtering footprint. This intentionally
magnifies mip artifacts. A production shader normally selects LOD from screen
space derivatives; manual `fract` wrapping in an atlas also needs careful
handling of gradients at repetition boundaries.

## Controls

| Key | Action |
| --- | --- |
| `Up` / `Down` | Increase / decrease mip level (0-7; array clamps at 6) |
| `G` | Toggle base-level half-texel inset for atlas material sampling |
| `T` | Toggle one / four repetitions per tile |
| `M` | Toggle material grid / storage view |
| `R` | Reset comparison |
| `H` | Show keyboard help |
| `Esc` | Quit |

No external images are needed. See the commented uploads in `src/main.cpp` and
the two addressing formulas in `shader/atlas_array.frag`.

Build from an already configured tree:

```sh
cmake --build build/windows --config Release --target Texture_AtlasArray
```

On Windows, run `bin/Basics/Release/Texture_AtlasArray.exe`. See the
[repository README](../../../README.md) for dependency setup and other platforms.
