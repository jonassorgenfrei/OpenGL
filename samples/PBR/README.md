# PBR Examples

Each example is a separate executable with its own source and shader files.

## [Lighting](Lighting/)

Introduces physically based shading with metallic and roughness material parameters.
The fragment shaders calculate direct lighting using a microfacet BRDF.
An untextured mode varies material parameters across a grid of spheres.
A textured mode reads albedo, normal, metallic, roughness, and ambient-occlusion maps.
The SHADED_STATE and TEXTURE definitions select the rendering mode and material set.
The example makes it possible to compare material response under the same point lights.

## [Diffuse irradiance](Diffuse_irradiance/)

Adds diffuse image-based lighting to the physically based sphere scene.
An HDR equirectangular image is first converted into an environment cubemap.
A convolution pass integrates incoming light and stores the result in an irradiance cubemap.
The PBR shader samples that irradiance using the surface normal for its diffuse ambient contribution.
A background pass displays the environment; I switches the environment/irradiance display option.
Keys 1, 2, and 3 select different HDR images for comparing the resulting illumination.

## [Specular IBL](Specular_IBL/)

Extends image-based lighting with a specular environment contribution using the split-sum approximation.
The setup converts an HDR image into a cubemap and computes diffuse irradiance.
A prefilter pass stores environment reflections for different roughness levels in cubemap mip levels.
A separate integration pass generates a two-channel BRDF lookup texture.
The PBR shaders combine these textures with material parameters and direct lighting.
Compile-time options select textured materials and diagnostic views of the environment or BRDF map.
