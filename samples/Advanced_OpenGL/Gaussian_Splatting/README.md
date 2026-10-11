# Gaussian Splatting

A compact introduction to the rendering part of 3D Gaussian splatting. It
loads a standard binary 3DGS PLY, sorts its anisotropic splats back-to-front on
the CPU, and draws them with one instanced call. The vertex shader projects the
full 3D covariance and evaluates degree-3 spherical harmonics for the current
view direction; the fragment shader evaluates the Gaussian falloff.

This sample illustrates the rendering and compositing stages rather than the
training pipeline. CPU sorting is intentionally retained to keep the code
readable; it uses camera-plane depth and only runs when the view changes.
Production renderers generally use a GPU sort.

Camera controls match the `Diffuse_irradiance` sample: `Alt` + left mouse
rotates, `Alt` + middle mouse pans, `Alt` + right mouse dollies, and the mouse
wheel changes the field of view. `P` switches between Gaussian splats and the
raw PLY points. Number keys `0` through `3` select the spherical-harmonic
degree, making the view-dependent terms easy to compare. `F2` enters
fullscreen, and `Esc` or the normal window close button quits. The cursor is
not captured and remains available outside the window.

Point mode deliberately draws one small circle for every PLY vertex without
the Gaussian footprint or alpha reconstruction. It makes the relationship
between the input samples and the reconstructed splat image easy to discuss.

The scene is loaded from `content/gaussian_splats/fox_statue.ply`. This file is
a byte-for-byte copy of the original CC0 `kitune1.ply`, with 279,199 records
and the standard properties:

```text
x y z
scale_0 scale_1 scale_2
rot_0 rot_1 rot_2 rot_3 opacity
f_dc_0 f_dc_1 f_dc_2
f_rest_0 ... f_rest_44
```

Nothing in the PLY is baked or converted on the CPU: scales are exponentiated,
opacity receives a sigmoid, and all SH coefficients are decoded in the vertex
shader. See the asset `SOURCE.md` for provenance and its verified checksum.
The renderer applies a 180-degree X-axis model transform to convert the
Scaniverse/3DGS Y-down, Z-forward convention to OpenGL's Y-up, Z-backward
convention; the stored PLY remains unchanged.
