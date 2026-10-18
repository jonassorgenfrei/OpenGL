# Shadow Examples

Each example is a separate executable with its own source and shader files.

## [Cascaded Shadow Mapping](Cascaded_Shadow_Mapping/)

Splits the camera's view range into cascades to improve directional-light shadow coverage.
Each cascade receives a light-space matrix and its own depth-map region or layer.
The source includes alternative implementations selected by the IMPLEMENTATION definition.
The scene shader chooses a cascade according to the fragment's distance from the camera.
F toggles depth-map display, N selects a debug layer, and C captures light matrices for visualization.
The example exposes how cascade placement distributes shadow-map detail across near and far geometry.

## [Point Shadows](Point_Shadows/)

Renders omnidirectional shadows from a point light using a depth cubemap.
A geometry shader sends scene triangles to the six cubemap faces in the depth pass.
The lighting pass compares the light-to-fragment distance with the stored depth.
Several shadow sampling techniques allow unfiltered and filtered results to be compared.
Space toggles shadows, T cycles techniques, and L toggles the visible light marker.
The example demonstrates shadow coverage in every direction around a local light source.

## [Shadow Mapping](Shadow_Mapping/)

Implements a two-pass shadow map for a scene containing a floor and cubes.
The first pass renders scene depth from the light's viewpoint into a texture.
The second pass transforms fragments into light space and compares their depths with that map.
Alternative sampling techniques demonstrate changes in shadow-edge appearance.
D toggles debug output, T cycles techniques, and P changes the peter-panning option.
The example makes depth bias, shadow-map sampling, and light-space projection visible in one scene.

## [Shadow Volumes](Shadow_Volumes/)

Demonstrates stencil shadow volumes with a cube casting a shadow onto a plane.
A geometry shader extrudes silhouette edges away from the light to form a closed volume.
The depth-fail stencil pass counts volume crossings behind visible scene surfaces.
A later lighting pass uses the stencil result to restrict direct illumination.
The sample includes controls for shadows, the light marker, volume visualization, and wireframe.
The visible volume option helps explain how geometric shadow boundaries relate to the final shadow.
