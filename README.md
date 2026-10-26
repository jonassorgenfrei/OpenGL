# OpenGL Samples

This repository contains a collection of annotated OpenGL examples. Most of
them are based on [LearnOpenGL](https://learnopengl.com/Introduction) and
[OGLDev](https://ogldev.org/).

Samples are grouped into:

- [Basics](samples/Basics/README.md)
- [Lighting](samples/Lighting/README.md)
- [Model](samples/Model/README.md)
- [PBR](samples/PBR/README.md)
- [Shadows](samples/Shadows/README.md)
- [Advanced OpenGL](samples/Advanced_OpenGL/README.md)

## Texture learning examples

Start with [Texture Basics](samples/Basics/Texture_Basics/), then explore:

- [Texture Atlas / Texture Array](samples/Basics/Texture_AtlasArray/): compare
  the same stone, grass, wood, and metal images in atlas rectangles and array
  layers side by side, with a permanent preview of the complete atlas at mip 0.
  Press `1` for material detail or `2` for mip bleeding;
  inspect storage with `M` and experiment with repetition and a base-level UV inset.
- [Texture Compression](samples/Basics/Texture_Compression/): compare a real
  photograph in RGBA8, DXT1/BC1, DXT3/BC2, DXT5/BC3, and three ASTC block sizes.
  Inspect alpha and compression artifacts, then compare payload savings with
  measured file-read and completed-upload times from prepared DDS/ASTC assets.
- [Virtual Texturing](samples/Basics/Texture_VirtualTexturing/): explore a real 16K
  NASA Blue Marble satellite map with 2.51 MiB of cache and page-table texture storage instead of
  a 1024 MiB full base image. Live overview/cache panels, residency colors, and
  pause/clear/budget controls make page streaming and its tradeoffs visible.
  Hover cache slots to highlight their corresponding image regions.

Each sample has on-screen keyboard help (`H`). The atlas/array and virtual
texturing READMEs include guided experiments. Both run on OpenGL 3.3. The
atlas/array sample generates its tiles procedurally;
virtual texturing reads prefiltered tiles from an included NASA image pack
without decoding the entire image into RAM.

After configuring, build these two targets with:

```sh
cmake --build build/windows --config Release --target Texture_AtlasArray Texture_VirtualTexturing
```

Use your configured build directory in place of `build/windows` on other platforms.

## General prerequisites

Both build workflows require **CMake 3.24 or newer** and a **C++17-capable
compiler**, along with the platform's OpenGL development libraries.

- **Windows:** Prefer **Visual Studio 2022 or newer** with the **Desktop
  development with C++** workload. Include the MSVC x64/x86 build tools,
  a Windows SDK, and **C++ CMake tools for Windows**. The bundled CMake can
  be used instead of a separate installation if it meets the version
  requirement. Visual Studio Build Tools with the same C++ components also
  work if you do not need the IDE.
- **Linux:** Use GCC or Clang with C++17 support, CMake, a build tool such as
  Make, and the OpenGL/X11 development packages listed in the Linux script.
- **Automatic dependency builds:** Git and Python 3 are also required to
  download dependency sources and generate GLAD. The scripts install missing
  prerequisites as described below. Manual builds use your existing libraries
  and generated GLAD sources, so Python is not required.

## Build everything

Choose either automatic dependency builds using the scripts below, or
[manually installed dependencies](#use-existing-dependencies). Both workflows
build the same samples. `OPENGL_FETCH_DEPENDENCIES` selects the dependency
source: `ON` (the default) downloads and builds libraries; `OFF` uses your
existing installations. Use separate build directories for the two workflows
to avoid reusing cached library paths or dependency build options.

The scripts download and build GLM 1.0.1, GLFW 3.4, Assimp 5.4.3,
FreeType 2.13.3, GLAD 0.1.36 and a pinned stb revision. CMake links these
dependencies to every sample automatically; no library paths or environment
variables are needed. Sources and libraries stay under `build/`. Binaries and
copied shaders are written under `bin/<sample group>/` (with a configuration
subdirectory on Windows). Downloads require internet access on the first run.

### Windows (x64)

From PowerShell in this repository:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1
```

The script finds an installed Visual Studio C++ toolchain and its bundled
CMake, or installs missing Git, CMake and Visual Studio 2022 Build Tools using
`winget`. Windows App Installer must be available if tools need installing;
the Visual Studio installer may request administrator access or a restart.
If Python is missing, the script downloads a portable Python 3.12 interpreter
inside the build directory. It does not change the system execution policy.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug -Jobs 4
```

Use `-SkipSystemDeps` when Git, CMake >= 3.24, Python 3 and the Visual Studio
C++ workload are already installed. `-BuildDirectory` selects another build
directory and `-CMakeArgs` accepts additional CMake options. Rerunning reuses
downloaded sources and compiled dependencies.
The script reuses an existing Visual Studio generator and selects a compatible
CMake executable, preferring the one recorded in the build cache and then
Visual Studio's bundled CMake. New build directories use the installed Visual
Studio version explicitly, regardless of `CMAKE_GENERATOR` or `PATH` defaults.

After configuration, open `build/windows/OpenGLSamples.slnx` in Visual Studio
(or `OpenGLSamples.sln` if that is what your CMake version generated). Select
`Release | x64` and set the desired sample as the startup project. If you
configured with `-B build`, open the solution in `build/` instead.

CMake reuses the script's portable Python when Visual Studio regenerates the
solution. GLAD generation selects UTF-8 explicitly, so Visual Studio does not
need to inherit Python environment settings from PowerShell.

### Linux

On Debian/Ubuntu (Ubuntu 24.04 or newer recommended):

```bash
bash scripts/build.sh
BUILD_TYPE=Debug BUILD_JOBS=4 bash scripts/build.sh
```

The script uses `sudo apt-get` to install the compiler, CMake, Git, Python,
pkg-config, and OpenGL/X11 development packages. It then builds the library
dependencies and all samples. CMake >= 3.24 is required. Linux builds use X11;
Wayland support is disabled. On other distributions, install equivalent
system packages and run `bash scripts/build.sh --skip-system-deps`.
Set `BUILD_DIR` to choose another build directory. Additional arguments after
`--skip-system-deps` are passed to CMake.

The default build parallelism is two jobs to limit memory use. Assimp builds
the OBJ, FBX, Collada and glTF importers; FreeType uses its built-in font
support without optional compression or shaping libraries. GLAD generates
OpenGL 4.6 core plus all extensions from its bundled specifications, including
the NVIDIA mesh shader extension required by the Mesh Shader sample.

### Use existing dependencies

For a manually managed installation, configure with
`-DOPENGL_FETCH_DEPENDENCIES=OFF` directly through CMake. This mode does not
download dependencies or require Python. The build scripts default to the
automatic workflow.

| Dependency | CMake input | Value |
| --- | --- | --- |
| GLM | `glm_DIR` | Directory containing `glmConfig.cmake` |
| GLFW | `glfw3_DIR` | Directory containing `glfw3Config.cmake` |
| Assimp 5.3 or newer | `assimp_DIR` | Directory containing `assimpConfig.cmake` |
| FreeType | `FREETYPE_*` or a discoverable installation prefix | Headers and library |
| GLAD 1.x | `glad_DIR` | Root containing `include/glad/glad.h` and `src/glad.c` |
| stb | `stb_DIR` | Directory containing `stb_image.h` |

The package directory names changed from the previous setup: `GLM_DIR` becomes
`glm_DIR`, `GLFW3_DIR` becomes `glfw3_DIR`, and `ASSIMP_DIR` becomes `assimp_DIR`.
The `glad_DIR`, `stb_DIR`, and `FREETYPE_*` inputs stay the same. Instead of
individual package directories, you can supply installation prefixes through
`CMAKE_PREFIX_PATH`. GLAD must use the 1.x API and include the extensions used
by the samples, including `GL_NV_mesh_shader`; GLAD 2 uses a different API.
`glad_DIR` and `stb_DIR` also accept environment variables as before.

Windows example (replace the paths with your installations):

```powershell
cmake -S . -B build/manual-windows -G "Visual Studio 17 2022" -A x64 `
  -DOPENGL_FETCH_DEPENDENCIES=OFF `
  -Dglm_DIR="D:/Libraries/glm/lib/cmake/glm" `
  -Dglfw3_DIR="D:/Libraries/glfw/lib/cmake/glfw3" `
  -Dassimp_DIR="D:/Libraries/assimp/lib/cmake/assimp-5.4" `
  -Dglad_DIR="D:/Libraries/glad" `
  -Dstb_DIR="D:/Libraries/stb" `
  -DFREETYPE_INCLUDE_DIR_ft2build="D:/Libraries/freetype/include/freetype2" `
  -DFREETYPE_INCLUDE_DIR_freetype2="D:/Libraries/freetype/include/freetype2" `
  -DFREETYPE_LIBRARY="D:/Libraries/freetype/lib/freetype.lib"
cmake --build build/manual-windows --config Release --parallel 2
```

On Windows, CMake copies the linked runtime DLLs beside each executable after
building, using the selected configuration (Debug or Release). This includes
Assimp and transitive DLL dependencies exposed by the packages' CMake targets.
Packages must describe shared libraries with their DLL locations; DLLs missing
from that metadata must be copied manually or made available through `PATH`.
Static dependencies need no DLL copying. Use libraries built for the same
architecture, compiler, and runtime configuration as the samples. For separate
FreeType Debug/Release libraries, use `FREETYPE_LIBRARY_DEBUG` and
`FREETYPE_LIBRARY_RELEASE` instead of `FREETYPE_LIBRARY`.

Linux example with dependencies already installed under `/opt/opengl-deps`:

```bash
cmake -S . -B build/manual-linux \
  -DOPENGL_FETCH_DEPENDENCIES=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/opengl-deps \
  -Dglad_DIR=/path/to/generated-glad \
  -Dstb_DIR=/path/to/stb
cmake --build build/manual-linux --parallel 2
```

Install the compiler, CMake >= 3.24, OpenGL and X11 development libraries
yourself for this workflow. To switch configurations, use `--config Debug`
with Visual Studio, or reconfigure with `-DCMAKE_BUILD_TYPE=Debug` on Linux.

## Continuous integration

[Build samples](.github/workflows/build.yml) runs these same scripts on pushes,
pull requests and manual dispatches. It compiles and links **all samples** in
Debug and Release on Ubuntu 24.04 and Windows Server 2022. It does not launch
the graphical examples: running them needs a display, a suitable OpenGL driver
and, for some samples, GPU-specific extensions.
