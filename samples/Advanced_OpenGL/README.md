# Advanced OpenGL Examples

Each example is a separate executable with its own source and shader files.

## [AdvancedDataGLSL](AdvancedDataGLSL/)

Demonstrates sharing transformation data between several shader programs with a uniform buffer.
A common uniform block stores the camera projection and view matrices.
The application binds that block to the same buffer binding point in each program.
Four cubes use different fragment shaders while sharing the same camera data.
The view matrix is updated in the buffer as the camera moves.
The example reduces repeated uniform updates when several programs need identical transformation data.

## [AdvancedOGL](AdvancedOGL/)

Combines several framebuffer, blending, and environment-mapping techniques in one scene.
Scene rendering uses an offscreen framebuffer whose color texture is displayed by a screen pass.
The setup includes depth testing, face culling, and blending for different scene objects.
Cubemap shaders provide a skybox and reflective or refractive object rendering.
Separate shaders keep ordinary scene shading, environment effects, and screen composition distinct.
The example shows how several OpenGL state settings and rendering passes fit together.

## [Anti Aliasing Off Screen](Anti_Aliasing_Off_Screen/)

Demonstrates multisample anti-aliasing with an application-managed framebuffer.
The scene is drawn into multisampled color and depth/stencil attachments.
A framebuffer blit resolves the multisample color result into a regular texture.
A fullscreen quad then displays the resolved texture on the window framebuffer.
This keeps anti-aliasing compatible with a rendering pipeline that uses intermediate targets.
The example separates multisample scene rendering, resolving, and screen presentation.

## [Anti Aliasing On Screen](Anti_Aliasing_On_Screen/)

Demonstrates multisample anti-aliasing directly on the window's default framebuffer.
The GLFW window setup requests a framebuffer with multiple samples per pixel.
The scene renders a cube using the usual model, view, and projection matrices.
Multisampling improves coverage at the cube's geometric edges.
The application enables multisample rasterization without a separate offscreen resolve pass.
This provides a smaller counterpart to the offscreen anti-aliasing example.

## [Billboards](Billboards/)

Generates camera-facing textured quads from points with a geometry shader.
The scene uses the quads to represent repeated objects above a ground surface.
A texture array provides multiple image layers for the billboard appearance and animation.
Alpha testing discards parts of the quads that lie outside the visible image shapes.
A toggles animation, U visualizes texture coordinates, and the arrow keys adjust the alpha threshold.
Wireframe mode exposes the generated geometry used to display the image-based objects.

## [Compute Shader](Compute_Shader/)

Uses a compute shader to update a water surface before drawing it in a scene.
The water helper manages alternating position and normal buffers for simulation updates.
The updated data is consumed by the water rendering pass alongside the ground and sky.
The application also creates a G-buffer and provides views of its intermediate results.
A pauses or resumes simulation, F1 toggles wireframe, and F3 toggles buffer visualization.
The example combines GPU simulation with a multi-pass scene renderer.

## [Compute Shader Particles](Compute_Shader_Particles/)

Simulates one million particles using a compute shader and shader storage buffers.
Separate buffers hold particle positions and velocities for the update pass.
The compute workgroups apply attraction, rotation, and random motion around a mouse-controlled attractor.
Boundary checks keep the simulated particles within the framebuffer area.
A memory barrier makes the updated positions available to the particle rendering pass.
Additive blending combines the rendered particles into a bright moving pattern.

## [Gaussian Splatting](Gaussian_Splatting/)

Loads a standard 3DGS PLY and renders anisotropic Gaussians with an instanced draw.
The CPU sorts splats by camera-plane depth, and shaders project their covariance
and evaluate view-dependent spherical-harmonic color. Premultiplied-alpha
blending combines the splats. P switches to raw point samples, 0 through 3
select the SH degree, and H shows the shared keyboard help.

## [Geometry Shader](Geometry_Shader/)

Introduces a geometry shader that turns each input point into a small house shape.
The vertex shader forwards the point position and its color through an interface block.
The geometry stage emits five vertices as a triangle strip with a roof at the top.
Most emitted vertices inherit the input color, while the roof tip receives a white color.
EmitVertex and EndPrimitive explicitly control the generated output geometry.
The example isolates primitive expansion without requiring an imported mesh.

## [Geometry Shader Exploding](Geometry_Shader_Exploding/)

Applies a geometry shader to the triangles of an imported nanosuit model.
The shader calculates a face direction and displaces each triangle to create an exploding appearance.
A time-dependent displacement animates the separation of the model's surface pieces.
A second geometry-shader program provides normal visualization for inspecting the mesh.
The E key controls the explosion option while the camera allows the result to be examined.
The example demonstrates manipulating complete primitives after vertex processing.

## [Instancing](Instancing/)

Draws a grid of one hundred colored quads with a single instanced draw call.
The base vertex buffer stores the geometry and colors shared by every quad.
A separate instance buffer supplies a different screen-space offset for each instance.
An attribute divisor advances the offset once per instance rather than once per vertex.
The vertex shader also uses gl_InstanceID to vary the size of the quads.
The example introduces instanced attributes without the complexity of a full 3D scene.

## [no Instancing Space](no_Instancing_Space/)

Renders a planet surrounded by an asteroid field using ordinary per-object drawing.
The application generates a collection of model matrices for the asteroid placements.
Each asteroid reuses the imported rock model with its own transform.
The render loop sends a model matrix and issues mesh drawing commands for each object.
The scene provides a counterpart to the Instancing_Space implementation.
Comparing the two sources shows where repeated draw calls and uniform updates can be replaced.

## [Instancing Space](Instancing_Space/)

Renders a large asteroid field around an imported planet using instanced drawing.
The application generates model matrices with different positions, scales, and rotations.
Those matrices are uploaded to an instance buffer and exposed as four vertex attributes.
Attribute divisors advance the matrix once for each asteroid instance.
Each rock mesh is drawn with glDrawElementsInstanced instead of a separate call per asteroid.
The example extends basic instancing to textured imported models in a 3D scene.

## [Mesh Shader](Mesh_Shader/)

Demonstrates a task and mesh shader pipeline using the GL_NV_mesh_shader extension.
The task stage launches mesh work, and the mesh stage generates vertices and triangle indices.
The supplied mesh shader builds a small colored shape from five vertices and four triangles.
Per-vertex and per-primitive data are passed to the fragment stage for coloring.
The application starts this pipeline with glDrawMeshTasksNV instead of a conventional mesh draw.
Running this example requires a driver and GPU that expose the NVIDIA mesh shader extension.

## [Order Independent Transparency Depth Peeling](Order_Independent_Transparency_Depth_Peeling/)

Renders overlapping transparent objects using repeated depth-peeling passes.
An initial opaque pass establishes the scene color and depth in framebuffer textures.
Subsequent passes select successive transparent layers using the previous layer's depth.
Alternating framebuffer targets hold the depth and color information needed for peeling.
The peeled layers are accumulated and composited with the opaque scene.
The arrow keys adjust the peeling-pass count, allowing quality and rendering cost to be compared.

## [Order Independent Transparency Weighted Blended](Order_Independent_Transparency_Weighted_Blended/)

Approximates order-independent transparency without sorting transparent geometry.
An opaque pass first captures the solid scene and its depth buffer.
Transparent fragments contribute to a weighted color accumulation buffer and a revealage buffer.
Separate blend functions update the two targets during the transparent pass.
A fullscreen composition pass combines those buffers with the opaque scene.
The example contrasts a fixed set of accumulation passes with repeated depth peeling.

## [Stencil Silhouette](Stencil_Silhouette/)

Uses the stencil buffer to draw an outline around textured scene objects.
The first object pass records stencil values wherever the visible objects are drawn.
A second pass draws enlarged versions of the objects with a single-color shader.
The stencil comparison rejects pixels already covered by the original object silhouettes.
The remaining fragments form an outline around each object's projected boundary.
Wireframe mode helps inspect the geometry used for the original and expanded object passes.

## [Tessellation](Tessellation/)

Demonstrates tessellation with triangle patches and vertex displacement from a height texture.
The tessellation control shader selects subdivision levels from the camera-to-vertex distances.
The tessellation evaluation shader interpolates positions, normals, and texture coordinates.
A height sample then offsets each generated vertex along the interpolated surface normal.
Q or E changes the displacement factor, and W toggles wireframe to expose the subdivisions.
The example separates patch subdivision from the surface detail introduced by displacement.

## [Tessellation Terrain](Tessellation_Terrain/)

Builds a terrain surface from a height map using tessellated quad patches.
A coarse grid supplies control points rather than the complete detailed terrain mesh.
The tessellation stages generate additional vertices and sample their heights from the texture.
The shader setup adjusts subdivision to represent the terrain at different viewing distances.
The USE_TESSELLATION definition selects between tessellated terrain and the alternative rendering path.
W toggles wireframe, making the generated terrain topology easier to inspect.

## [Transform Feedback](Transform_Feedback/)

Demonstrates capturing shader outputs into buffers for later use.
Compile-time MODE options select a square-root calculation, geometry-shader output, or animated points.
The default point mode updates positions in response to the mouse location.
Captured positions and velocities are read back with glGetBufferSubData after each frame.
The CPU copies those values into the input vertex buffer for the following update.
The example introduces output capture and readback before the separate particle sample keeps state on the GPU.

## [Transform Feedback Particles](Transform_Feedback_Particles/)

Updates a particle system on the GPU using transform feedback.
The update shaders read particle state and write the next state into an alternate buffer.
Transform feedback objects retain the number of generated particles for later drawing.
A separate geometry shader expands particles into camera-facing textured billboards.
The active sample uses a fireworks texture and a system initialized at the scene origin.
The example separates particle simulation, state storage, and billboard rendering into distinct stages.
