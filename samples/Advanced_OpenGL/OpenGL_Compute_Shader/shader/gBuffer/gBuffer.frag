#version 430 core

// ----------------------------------------------------------------------------
//
// Typen
//
// ----------------------------------------------------------------------------

/**
 * Material-Parameter
 */
struct Material {
    vec4 color;                 /**< Beleuchtungsunabhängige Farbe */
    float ambientReflection;    /**< Ambienter Reflexionskoeffizient */
    float diffuseReflection;    /**< Diffuser Reflexionskoeffizient */
    float specularReflection;   /**< Spekularer Reflexionskoeffizient */
    float shininess;            /**< Wahrgenommene Glattheit der Oberfläche */
    bool hasTexture;            /**< Gibt an, ob das Material Reflexionskoeffizienten aus einer Textur bezieht */
};

// ----------------------------------------------------------------------------
//
// Attribute
//
// ----------------------------------------------------------------------------

in vec3 fWorldNormal;                               /**< Normale des Vertexes im Welt-Koordinatensystem */
in vec2 fTexCoord;                                  /**< Textur-Koordinate */

layout (location = 0) out vec3 FragColor;           /**< Farbe des Fragments (auf Attachment 0) */
layout (location = 1) out vec4 FragReflection;      /**< Reflexionseigenschaften des Materials */
layout (location = 2) out vec3 FragNormal;          /**< Normale des Fragments im Welt-Koordinatensystem */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

/* auto location */  uniform Material ObjectMaterial;           /**< Material das gerendert werden soll */

layout (binding = 0) uniform sampler2D ObjectMaterialTexture;   /**< Textur mit Reflexionskoeffizienten */

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Fragment-Shader
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
