# Basic Examples

These examples introduce shaders, textures, transformations, cameras, and text rendering.
Each example is a separate executable with its own source and shader files.

## [Transformation](Transformation/)

Demonstrates translation, rotation, and scaling with GLM transformation matrices.
Two textured rectangles reuse the same geometry with different transformations.
The first rectangle rotates over time around its local origin and is translated across the screen.
The second rectangle changes scale using a sine function and is placed at another position.
A matrix uniform transfers each transformation to the vertex shader before drawing.
The example shows how matrix multiplication order determines the resulting motion.

## [Shader](Shader/)

Introduces GLSL vertex and fragment shaders using triangles and an indexed rectangle.
Vertex array and buffer objects describe positions and per-vertex colors.
Separate fragment shaders demonstrate interpolated colors, uniform colors, and position-based output.
A time-dependent uniform animates the green component of one triangle's color.
Another uniform shifts a triangle horizontally in the vertex shader.
The example also contrasts drawing vertex arrays with drawing indexed geometry.

## [Coordinate System](CoordinateSystem/)

Extends the earlier examples to a scene containing ten textured cubes.
Separate model, view, and projection matrices transform vertices into clip space.
Each cube receives its own model matrix for placement and animated rotation.
A fixed view translation moves the scene in front of the camera.
Perspective projection uses the current window aspect ratio, and depth testing resolves visibility.
Additional colored primitives retain a connection to the earlier shader examples.

## [Texture Basics](Texture_Basics/)

Demonstrates loading image files with stb_image and uploading them as OpenGL textures.
A container image and a face image are assigned to separate texture units.
Texture coordinates map both images onto an indexed rectangle.
The fragment shader combines the images using a mixing uniform.
The texture setup includes repeat wrapping, linear filtering, and mipmap generation.
Use Up and Down to change the blend, R to reset it, and H to show keyboard help.

## [Texture Compression](Texture_Compression/)

Compares a real landscape photograph against DXT1/BC1, DXT3/BC2, DXT5/BC3,
and ASTC 4x4, 8x8, and 12x12 using prepared DDS/ASTC assets.
A labeled alpha ramp exposes RGB-only BC1, explicit BC2 alpha, and interpolated BC3 alpha.
Side-by-side reference, alpha/error views, zoom, and a block grid reveal quality differences.
The bottom table shows payload size, ratio, savings, and measured file-read, completed-upload,
and total loading times from seven warm-cache trials; encoding happens offline.
Unsupported GPU formats use labeled RGBA8 previews with compressed upload/load timings marked N/A.
Use 1-7 for formats, A for alpha, D for error, Z/wheel for zoom, B for blocks, T to remeasure, and H for help.

## [Texture Atlas / Texture Array](Texture_AtlasArray/)

Compares four identical tiles packed into one 2D atlas or four texture-array layers.
Side-by-side material grids show UV-rectangle addressing versus layer selection.
Adjustable mip levels and repetition expose atlas filtering bleed and isolated array filtering.
A half-texel inset demonstrates a base-level fix and its limitations at coarser mips.
The storage view shows the packed image and separate layers; each uses 64 KiB at the base level.
Use Up/Down for mip level, G for inset, T for repetition, M for storage, and H for help.

## [Virtual Texturing](Texture_VirtualTexturing/)

Explore a real 16384 x 16384 NASA Blue Marble satellite map using a bounded 11 x 11 physical page cache.
The on-screen comparison shows a 1024 MiB full base image versus 2.51 MiB of cache and page table.
Click the overview to jump between regions; a live cache preview reveals slot reuse.
Hover cache slots to mark their image footprints in cyan and inspect page coordinates and mip level.
Prefiltered image tiles stream from an included disk pack through a bounded CPU tile cache.
Optional green/orange residency colors distinguish requested detail from coarse fallback during streaming.
Pause and clear detail to observe fallback, then resume with one or four uploads per frame.
Use WASD/arrows to pan, wheel or Q/E to zoom, Space to pause, C to clear, B for the budget,
V for residency colors, O for overview, R for the Italy close-up, and H for help.
The sample README includes a guided experiment and explains costs and implementation limits.

## [Camera](Camera/)

Adds an interactive camera to a scene containing ten rotating, textured cubes.
The shared Camera class supplies the view matrix and stores the viewing direction.
Use W, A, S, and D to move through the scene, and move the mouse to look around.
Mouse-wheel input changes the camera zoom used by the perspective projection.
Keyboard movement uses elapsed frame time to keep movement speed independent of frame rate.
Depth testing handles overlapping cubes; F2 switches to fullscreen and Esc closes the sample.

## [Text](Text/)

Demonstrates drawing text with FreeType and the shared TextRenderer module.
The renderer loads GO-Regular.ttf and uploads rasterized glyphs as individual textures.
Glyph sizes, bearings, and advances determine the placement and spacing of character quads.
An orthographic projection positions the text in window pixel coordinates.
Alpha blending combines glyph coverage with the background, while each string supplies its color and scale.
The sample updates the text projection when resized; F2 switches to fullscreen and Esc closes it.
