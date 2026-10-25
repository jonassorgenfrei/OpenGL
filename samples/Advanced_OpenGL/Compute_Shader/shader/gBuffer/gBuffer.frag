#version 430 core

// ----------------------------------------------------------------------------
//
// Types
//
// ----------------------------------------------------------------------------

/**
 * Material-Parameter
 */
struct Material {
    vec4 color;                 /**< Unlit base color */
    float ambientReflection;    /**< Ambient reflection coefficient */
    float diffuseReflection;    /**< Diffuse reflection coefficient */
    float specularReflection;   /**< Specular reflection coefficient */
    float shininess;            /**< Perceived surface smoothness */
    bool hasTexture;            /**< Whether the material reads reflection coefficients from a texture */
};

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec3 fWorldNormal;                               /**< Vertex normal in world space */
in vec2 fTexCoord;                                  /**< Texture coordinate */

layout (location = 0) out vec3 FragColor;           /**< Fragment color written to attachment 0 */
layout (location = 1) out vec4 FragReflection;      /**< Material reflection properties */
layout (location = 2) out vec3 FragNormal;          /**< Fragment normal in world space */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

/* auto location */  uniform Material ObjectMaterial;           /**< Material to render */

layout (binding = 0) uniform sampler2D ObjectMaterialTexture;   /**< Texture containing reflection coefficients */

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Entry point for the fragment shader
 */
void main() {
    vec2 texCoords = vec2(fTexCoord.x, 1 - fTexCoord.y);
    vec4 materialColor = ObjectMaterial.hasTexture ? texture(ObjectMaterialTexture, texCoords) : ObjectMaterial.color;

    FragColor       = materialColor.rgb;
    FragReflection  = vec4(ObjectMaterial.ambientReflection,
                           ObjectMaterial.diffuseReflection,
                           ObjectMaterial.specularReflection,
                           ObjectMaterial.shininess);
    FragNormal      = 0.5*fWorldNormal + vec3(0.5);
}
