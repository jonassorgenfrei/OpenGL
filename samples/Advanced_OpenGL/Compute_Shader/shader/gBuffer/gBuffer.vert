#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                /**< Vertex position in model space */
layout (location = 1) in vec3 vNormal;                  /**< Vertex normal in model space */
layout (location = 2) in vec2 vTexCoord;                /**< Vertex texture coordinate */

out vec3 fWorldNormal;                                  /**< Vertex normal in world space */
out vec2 fTexCoord;                                     /**< Vertex texture coordinate */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ModelMatrix;         /**< Transforms from model space to world space */
layout (location = 1) uniform mat4 ViewMatrix;          /**< Transforms from world space to view space */
layout (location = 2) uniform mat4 ProjectionMatrix;    /**< Transforms from view space to clip space */
layout (location = 3) uniform mat3 NormalMatrix;        /**< Transforms normals from model space to world space */

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the vertex shader
 */
void main() {
    fWorldNormal = normalize(NormalMatrix*vNormal);
    fTexCoord = vTexCoord;
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}
