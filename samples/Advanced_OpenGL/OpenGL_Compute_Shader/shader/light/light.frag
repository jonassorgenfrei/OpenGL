#version 430 core

// ----------------------------------------------------------------------------
//
// Typen
//
// ----------------------------------------------------------------------------

/**
 * Parameter einer gerichteten Lichtquelle
 */
struct DirectionalLight {
    vec3 direction;             /**< Austrahlvektor der Lichtquelle */
    vec4 ambientColor;          /**< Anteil der Lichtquelle an der ambienten Beleuchtung */
    vec4 diffuseColor;          /**< Anteil der Lichtquelle an der diffusen Beleuchtung */
    vec4 specularColor;         /**< Anteil der Lichtquelle an der spekularen Beleuchtung */
};

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

in vec4 fDcPosition;

out vec4 FragColor;                         /**< Beleuchtung des Fragments */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 ViewerInverseViewProjectionMatrix;   /**< Transformation vom Clipping-Koordinatensystem des Betrachters ins Welt-Koordinatensystem */
layout (location = 1) uniform mat4 ViewerViewMatrix;                    /**< Transformation vom Welt-Koordinatensystem ins Betrachter-Koordinatensystem */
//layout (location = 2) uniform mat4 ShadowMapViewMatrix;                 /**< Transformation vom Welt- ins Betrachter-Koordinatensystem der Lichtquelle */
//layout (location = 3) uniform mat4 ShadowMapProjectionMatrix;           /**< Transformation vom Betrachter- ins Clipping-Koordinatensystem der Lichtquelle */
//layout (location = 4) uniform int UseSSAO;                              /**< Bestimmt, ob SSAO eingesetzt wird */
/* auto location */ uniform DirectionalLight Sun;                       /**< Gerichtete Lichtquelle in der Szene */

layout (binding = 0) uniform sampler2D ColorBuffer;
layout (binding = 1) uniform sampler2D ReflectionBuffer;
layout (binding = 2) uniform sampler2D NormalBuffer;
layout (binding = 3) uniform sampler2D DepthBuffer;
//layout (binding = 4) uniform sampler2D ShadowMap;

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Division durch die w-Komponente
 *
 * @param                       v 4D-Vektor, für die w-Division
 *
 * @return 3D-Vektor nach der w-Division
 */
vec3 wdiv(vec4 v) {
    return v.xyz / v.w;
}

/**
 * Berechnet die Beleuchtung eines Punktes durch eine gerichtete Lichtquelle
 *
 * @param materialColor         Farbe des Materials
 * @param directionalLight      Informationen über die Lichtquelle
 * @param viewMatrix            ViewMatrix des Betrachters
 * @param worldNormal           Normale des Punktes im Welt-Koordinantensystem
 * @param worldPosition         Position des Punktes im Welt-Koordinatensystem
 * @param attenuation           Abschwächung des Lichts durch Verdeckung (Shadow-Mapping)
 * @param ssao                  Wert der Screen Space Ambient Occlusion
 *
 * @return Belechtung des Punktes
 */
vec4 shadeDirectionalLight(Material material,
                           vec4 materialColor,
                           DirectionalLight directionalLight,
                           mat4 viewMatrix,
                           vec3 worldNormal,
                           vec3 worldPosition,
                           float attenuation)
{
    vec3 cameraPosition = wdiv(inverse(viewMatrix)*vec4(0, 0, 0, 1));
    vec3 cameraDirection = normalize(cameraPosition - worldPosition);
    vec3 reflectionDirection = normalize(reflect(directionalLight.direction, worldNormal));

    float s_diff = max(0, dot(worldNormal, normalize(directionalLight.direction)));
    float s_spec = pow(max(0, dot(reflectionDirection, cameraDirection)), material.shininess);

    vec4 i_amb  = materialColor*material.ambientReflection*directionalLight.ambientColor;
    vec4 i_diff = s_diff*materialColor*material.diffuseReflection*directionalLight.diffuseColor;
    vec4 i_spec = s_spec*materialColor*material.specularReflection*directionalLight.specularColor;

    return i_amb + attenuation*(i_diff + i_spec);
}

/**
 * Inverse Depth-Range Transformation
 *
 * @param depth                 Tiefe aus dem Depth-Buffer
 *
 * @return Tiefe im Clipping-Koordinatensystem
 */
float inverseDepthRangeTransformation(float depth) {
    return (2.0 * depth - gl_DepthRange.near - gl_DepthRange.far) /
            (gl_DepthRange.far - gl_DepthRange.near);
}

/**
 * Einsprungpunkt für den Fragment-Shader
 */
void main() {
    vec3 ndcPosition = fDcPosition.xyz / fDcPosition.w;
    vec2 bufferTexCoord = 0.5*ndcPosition.xy + vec2(0.5);

    // Buffer auslesen
    vec3 color      = texture(ColorBuffer, bufferTexCoord).rgb;
    vec4 reflection = texture(ReflectionBuffer, bufferTexCoord);
    vec3 normal     = texture(NormalBuffer, bufferTexCoord).rgb;

    Material material;
    material.color              = vec4(color, 1);
    material.ambientReflection  = reflection.r;
    material.diffuseReflection  = reflection.g;
    material.specularReflection = reflection.b;
    material.shininess          = reflection.a;
    material.hasTexture         = false;

    vec3 worldNormal = 2*normal - vec3(1);

    if (dot(worldNormal, worldNormal) < 0.001) {
        discard;
    }

    float depthFromDepthBuffer = texture(DepthBuffer, bufferTexCoord).r;
    float depth = inverseDepthRangeTransformation(depthFromDepthBuffer);
    vec4 fragmentDcPosition = vec4(2*bufferTexCoord - vec2(1), depth, 1);
    vec4 fragmentWorldPosition4 = ViewerInverseViewProjectionMatrix*fragmentDcPosition;
    vec3 fragmentWorldPosition  = wdiv(fragmentWorldPosition4);

    FragColor = shadeDirectionalLight(material,
                                      vec4(color, 1),
                                      Sun,
                                      ViewerViewMatrix,
                                      worldNormal,
                                      fragmentWorldPosition,
                                      1);
    //FragColor = vec4(normal, 1);
}
