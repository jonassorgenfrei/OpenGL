# Lighting Examples

Each example is a separate executable with its own source and shader files.

## [Advanced Lighting](Advanced_Lighting/)

Compares lighting models on a textured surface viewed with an interactive camera.
The fragment shader supports Phong and Blinn-Phong specular highlights.
An additional Oren-Nayar option demonstrates a rough diffuse reflection model.
Light position, surface normals, and the viewing direction determine the shading.
Press B to toggle Blinn-Phong and O to toggle the Oren-Nayar option.
The example helps compare how diffuse and specular choices change the same scene.

## [Bloom](Bloom/)

Adds a glow around bright objects using an HDR scene and a separate brightness buffer.
Multiple render targets capture the scene color and its bright regions in one pass.
Alternating horizontal and vertical blur passes spread the bright image over neighboring pixels.
A final screen pass combines the blurred image with the original scene and applies exposure.
Press Space to toggle bloom and Q or E to adjust exposure.
The example separates scene rendering, filtering, and final image composition.

## [Physically Based Bloom](Physically_Based_Bloom/)

Contains a mip-chain bloom implementation alongside the original bloom rendering path.
The BloomRenderer class defines progressively smaller downsample targets and additive upsampling.
Its first downsample can use Karis averaging to reduce the influence of very bright samples.
The active main loop still extracts bright regions and uses horizontal and vertical ping-pong blur.
The mip-chain renderer is defined but is not currently connected to that main loop.
Space toggles the active bloom effect, and Q or E changes exposure.

## [Colors](Colors/)

Introduces the relationship between object colors and a light color using a cube scene.
The object shader combines vertex color with the light-color uniform and a texture contribution.
A separate shader draws a visible representation of the light source.
Model, view, and projection matrices place the cubes in a perspective scene.
W, A, S, and D move the camera, while mouse input changes the viewing direction.
The example provides a simple starting point before normal-based lighting calculations.

## [Deferred Shading](Deferred_Shading/)

Separates geometry rendering from the calculation of lighting contributions.
The geometry pass stores positions, normals, and material information in a G-buffer.
Later passes read those textures to shade the visible surfaces.
The source includes LearnOpenGL and OGLDev paths selected by the LEARNOPENGL definition.
The OGLDev path uses stencil-tested point-light volumes and additive light accumulation.
Press D to toggle the debug view and inspect the intermediate rendering data.

## [Forward Plus](Forward_Plus/)

Demonstrates tiled Forward+ lighting on an imported Sponza scene.
A depth pass provides the information needed to determine which lights affect each screen tile.
A compute shader constructs light lists that the forward fragment shader reads during shading.
This limits each fragment's lighting loop to the relevant lights in its tile.
M toggles the light-count heatmap, T shows tiles, and L toggles light visualization.
The sample combines compute shaders, shader storage buffers, and forward rendering.

## [Gamma Correction](Gamma_Correction/)

Compares lighting calculations with and without gamma correction on a textured surface.
The sample loads ordinary and sRGB versions of the same image.
Its shader changes the lighting and output treatment according to the gamma setting.
Several lights with different intensities make the difference visible across the surface.
Press Space to toggle gamma correction and B to switch the specular lighting option.
The example connects texture color-space handling with the final displayed result.

## [Gooch Model](Gooch_Model/)

Demonstrates Gooch shading as a stylized alternative to conventional surface lighting.
The fragment shader combines cool and warm colors according to the surface orientation.
A narrow highlight term adds a bright accent near the reflected viewing direction.
Normals, light position, and eye position are passed to the shader for each object.
A separate light marker helps relate the color transition to the illumination direction.
Camera movement allows the highlight and warm-to-cool transition to be inspected from different angles.

## [HDR](HDR/)

Renders a brightly lit scene into a floating-point framebuffer before displaying it.
The intermediate color buffer preserves lighting values beyond the display's usual range.
A fullscreen pass maps those values into the final image using an exposure setting.
The scene contains a textured tunnel with lights of different intensities.
Press Space to toggle HDR tone mapping and Q or E to adjust exposure.
The example shows why storing scene lighting and presenting an image are separate steps.

## [Lights](Lights/)

Combines several light types in a scene of textured cubes.
The shaders calculate ambient, diffuse, and specular contributions from surface normals.
Directional lights, attenuated point lights, and a spotlight have separate parameter sets.
Material textures supply surface color and control the appearance of highlights.
Visible light-source cubes help locate the point lights in the scene.
The example organizes reusable light and material settings for a forward lighting pass.

## [Normal Mapping](Normal_Mapping/)

Adds small surface detail by sampling a normal map instead of using only mesh normals.
The quad setup computes tangent and bitangent vectors from positions and texture coordinates.
The vertex shader transforms lighting information into the surface's tangent-space basis.
The fragment shader decodes the sampled normal and uses it for per-fragment lighting.
A rotating textured quad makes the changing highlights easy to inspect.
The added detail changes shading while the underlying quad geometry remains simple.

## [Parallax Mapping](Parallax_Mapping/)

Creates apparent surface depth by offsetting texture coordinates using a height map.
The shader calculates the viewing direction in tangent space alongside the normal mapping data.
Height samples determine where the diffuse and normal textures are evaluated.
The effect changes with the viewing angle, creating the impression of relief on a flat quad.
Press Q or E to adjust the height scale and W to toggle wireframe rendering.
The example contrasts texture-coordinate displacement with actually adding mesh geometry.

## [SSAO](SSAO/)

Approximates ambient occlusion from the visible positions and normals in a G-buffer.
A sampling kernel tests nearby positions to estimate how much surrounding geometry blocks ambient light.
A small noise texture varies the sampling orientation across the screen.
An optional blur pass smooths the occlusion result before the final lighting composition.
B toggles blur, D shows debug output, and Q or E changes the sample radius.
The default LearnOpenGL path also offers Hammersley sampling through the N key.
