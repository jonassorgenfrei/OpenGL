#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Vertex position in model space */
layout (location = 1) in vec3 vNormal;

out vec3 fWorldNormal;
out vec4 fWorldPosition;
out vec4 fDcPosition;
out float fWaterVelocity;

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ModelMatrix;             /**< Transforms from model space to world space */
layout (location = 1) uniform mat4 ViewMatrix;              /**< Transforms from world space to view space */
layout (location = 2) uniform mat4 ProjectionMatrix;        /**< Transforms from view space to clip space*/
layout (location = 3) uniform mat3 NormalMatrix;        /**< Transforms from view space to clip space*/

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fWorldNormal = NormalMatrix*vNormal;

    fWaterVelocity = vPosition.w;

    vec4 worldPosition = ModelMatrix*vec4(vPosition.xyz, 1);
    fWorldPosition = worldPosition;

    vec4 dcPosition = ProjectionMatrix*ViewMatrix*worldPosition;
    fDcPosition = dcPosition;
    gl_Position = dcPosition;
}
