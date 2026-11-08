# Noise types

This sample compares four procedural noise variants in a fragment shader:

- **Value noise** interpolates random scalar values stored conceptually at grid corners.
- **Perlin noise** interpolates the dot products between random corner gradients and
  the point-to-corner vectors. This removes the blocky extrema typical of value noise.
- **Fractal Perlin noise (fBm)** combines five Perlin octaves. Each octave increases
  frequency and decreases amplitude to add detail at several spatial scales.
- **Animated 3D Perlin noise** evaluates layered 3D noise with time as the third
  coordinate. The result changes continuously rather than translating a fixed 2D image.

All noise is generated procedurally on the GPU; the sample does not read a noise
texture. The quintic fade curve gives the interpolation zero first and second
derivatives at lattice boundaries.

| Key | Action |
| --- | --- |
| `1` | Show value noise |
| `2` | Show Perlin gradient noise |
| `3` | Show fractal Perlin noise (fBm) |
| `4` | Show animated 3D Perlin noise |
| Up / Down | Increase / decrease spatial scale |
| `Space` | Pause or resume animation |
| `R` | Restore Perlin noise, scale, and animation time |
| `H` | Show or hide keyboard help |
| `Esc` | Quit |

The window title displays the selected noise type, spatial scale, and pause state.
