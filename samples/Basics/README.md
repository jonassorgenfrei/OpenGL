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

Compares a generated RGBA8 image with DXT1/BC1 and ASTC 4x4 representations.
Small teaching encoders generate compressed blocks that are uploaded with glCompressedTexImage2D.
The application checks extension support and whether the driver accepts each compressed upload.
Unsupported or rejected formats fall back to the uncompressed source texture.
The console reports format support and payload sizes, while the window title identifies fallback modes.
Keys 1, 2, and 3 select the source, DXT1, and ASTC views; H shows keyboard help.

## [Virtual Texturing](Texture_VirtualTexturing/)

Displays a large logical texture through a bounded physical page cache and a page table.
The visible region determines which mip level and pages need to be resident.
Streaming uploads are limited per frame, and least-recently-used pages are replaced as needed.
Permanently resident coarse mip levels provide a fallback while detailed pages are missing.
The window title reports cache occupancy, upload activity, active mip, and zoom level.
Use WASD or arrows to pan, the wheel or Q/E to zoom, R to reset, and H for help.

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
