#version 430 core

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 vPosition;                /**< Position des Vertexes im Modell-Koordinatensystem */
layout (location = 1) in vec3 vNormal;                  /**< Normale des Vertexes im Modell-Koordinatensystem */
layout (location = 2) in vec2 vTexCoord;                /**< Textur-Koordinate des Vertexes */

out vec3 fWorldNormal;                                  /**< Normale des Vertexes im Welt-Koordinatensystem */
out vec2 fTexCoord;                                     /**< Textur-Koordinate des Vertexes */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ModelMatrix;         /**< Transformation vom Modell- ins Welt-Koordinatensystem */
layout (location = 1) uniform mat4 ViewMatrix;          /**< Transformation vom Welt- ins Kamera-Koordinatensystem */
layout (location = 2) uniform mat4 ProjectionMatrix;    /**< Transformation vom Kamera-Koordinatensystem ins Clipping-Koordinatensystem */
layout (location = 3) uniform mat3 NormalMatrix;        /**< Transformation der Normalen vom Modell- ins Welt-Koordinatensystem */

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Vertex-Shader
 */
void main() {
    fWorldNormal = normalize(NormalMatrix*vNormal);
    fTexCoord = vTexCoord;
    gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vPosition;
}
