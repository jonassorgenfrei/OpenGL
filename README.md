# OpenGL Samples

This repository contains a collection of annotated OpenGL examples. Most of
them are based on [LearnOpenGL](https://learnopengl.com/Introduction) and
[OGLDev](https://ogldev.org/).

Samples are grouped into:

- [Basics](samples/Basics/README.md)
- Lighting
- Model
- PBR
- Shadows
- Advanced OpenGL

## Dependencies

Install or build these dependencies before configuring the project:

| Dependency | CMake input | Value |
| --- | --- | --- |
| GLM | `GLM_DIR` | Directory containing `glmConfig.cmake` |
| GLFW | `GLFW3_DIR` | Directory containing `glfw3Config.cmake` |
| Assimp | `ASSIMP_DIR` | Directory containing `assimpConfig.cmake` |
| GLAD 1.x | `glad_DIR` | Root containing `include/glad/glad.h` and `src/glad.c` |
| FreeType | `FREETYPE_*` or a discoverable installation prefix | FreeType headers and library |
| stb | `stb_DIR` | Directory containing `stb_image.h` |

This project uses the GLAD 1.x API (`glad/glad.h`, `glad.c`, and
`gladLoadGLLoader`). A default GLAD 2 source tree uses different file names and
is not compatible without source changes. Some examples, such as Mesh Shader,
also require the corresponding GLAD extensions to have been generated.
Point `glad_DIR` directly at one generated GLAD directory, not at a parent
directory containing several GLAD variants.

## Configure and build on Windows

Run the following commands from the repository root in PowerShell. Replace the
example paths with the locations of your dependencies:

```powershell
cmake --fresh -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -Dglad_DIR="D:/Libraries/glad" `
  -Dstb_DIR="D:/Libraries/stb" `
  -DGLM_DIR="D:/Libraries/glm/build" `
  -DGLFW3_DIR="D:/Libraries/glfw/lib/cmake/glfw3" `
  -DASSIMP_DIR="D:/Libraries/assimp/lib/cmake/assimp" `
  -DFREETYPE_INCLUDE_DIR_ft2build="D:/Libraries/freetype/include/freetype2" `
  -DFREETYPE_INCLUDE_DIR_freetype2="D:/Libraries/freetype/include/freetype2" `
  -DFREETYPE_LIBRARY="D:/Libraries/freetype/lib/freetype.lib"

cmake --build build --config Release
```

`--fresh` removes the existing CMake cache before configuration. This is useful
when changing the generator or architecture. It is not required for normal
reconfiguration.

`glad_DIR` and `stb_DIR` can alternatively be supplied as environment
variables:

```powershell
$env:glad_DIR = "D:/Libraries/glad"
$env:stb_DIR = "D:/Libraries/stb"
cmake -S . -B build
```
