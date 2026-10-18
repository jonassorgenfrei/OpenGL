# Model Examples

Each example is a separate executable with its own source and shader files.

## [Basic](Basic/)

Introduces rendering imported meshes through the shared Model and Mesh modules.
The active asset is a minecart model loaded from an OBJ file with its associated materials.
The model loader supplies vertex attributes, indices, and material textures to the drawing code.
The fragment shader combines directional, point, and spotlight contributions.
Separate light-source geometry makes the scene's lighting arrangement visible.
The example connects model import with the same shader and camera workflow used for simple primitives.

## [Advanced Model](Advanced_Model/)

Extends imported-model rendering with a cubemap environment and a skybox.
The active scene loads the nanosuit model and its material textures.
Directional, point, and spotlight calculations provide the direct lighting contributions.
The model shader also samples the environment for a reflection contribution.
A separate skybox pass displays the surrounding cubemap behind the scene.
The example combines imported geometry, multiple light types, and environment mapping.

## [Skeletal Animation](Skeletal_Animation/)

Loads and animates a skinned character from the dancing vampire Collada asset.
The animation data contains a bone hierarchy and keyframes for the character's motion.
The Animator updates the animation time and calculates the final bone transformations each frame.
The vertex shader blends bone matrices using the bone indices and weights attached to each vertex.
The animated positions are then rendered with the model's surface textures.
W toggles wireframe, and F toggles frame-rate reporting for inspecting the running animation.
