#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                    /**< Position des Vertexes im Modell-Koordinatensystem */
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

layout (location = 0) uniform mat4 ModelMatrix;             /**< Transformation vom Modell- ins Welt-Koordinatensystem */
layout (location = 1) uniform mat4 ViewMatrix;              /**< Transformation vom Welt- ins Kamera-Koordinatensystem */
layout (location = 2) uniform mat4 ProjectionMatrix;        /**< Transformation vom Kamera- ins Clipping-Koordinatensystem*/
layout (location = 3) uniform mat3 NormalMatrix;        /**< Transformation vom Kamera- ins Clipping-Koordinatensystem*/

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
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
