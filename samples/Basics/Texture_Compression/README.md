# Texture compression

This sample compares a generated RGBA8 image with DXT1/BC1 and ASTC 4x4 GPU
block-compressed representations. It implements
[issue #7](https://github.com/jonassorgenfrei/OpenGL/issues/7).

| Key | Mode |
| --- | --- |
| `1` | RGBA8 source texture |
| `2` | DXT1 / BC1, encoded into 4x4 64-bit blocks |
| `3` | ASTC 4x4, encoded into 128-bit LDR void-extent blocks |
| `Esc` | Quit |

DXT1 and ASTC are uploaded with `glCompressedTexImage2D`. If the active OpenGL
driver does not expose the corresponding extension, that mode uses the RGBA8
texture as a safe fallback. The console distinguishes an unavailable extension
from a rejected upload and prints the active renderer; the window title marks
fallback modes.

The compact encoders favor readable teaching code. Production applications
should normally use a mature offline encoder with higher-quality searches.
