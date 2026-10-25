#version 430 core

// ----------------------------------------------------------------------------
//
// Konstanten
//
// ----------------------------------------------------------------------------

const float PI  = 3.14159265358979;
const float TAU = 2*PI;

// ----------------------------------------------------------------------------
//
// Types
//
// ----------------------------------------------------------------------------

/**
 * Parameters of a directional light
 */
struct DirectionalLight {
    vec3 direction;             /**< Direction in which the light travels */
    vec4 ambientColor;          /**< Ambient contribution of the light */
    vec4 diffuseColor;          /**< Diffuse contribution of the light */
    vec4 specularColor;         /**< Specular contribution of the light */
};

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

in vec3 fWorldNormal;
in vec4 fWorldPosition;
in vec4 fDcPosition;
in float fWaterVelocity;

out vec4 FragColor;                             /**< Fragment color */

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

layout (location = 1) uniform mat4 ViewMatrix;              /**< Transforms from world space to view space */
// HINT: vertex shader
layout (location = 4) uniform mat4 ViewerInverseViewProjectionMatrix;   /**< Transforms from viewer clip space to world space */
layout (location = 5) uniform mat4 ViewerViewMatrix;                    /**< Transforms from world space to view space */
//layout (location = 2) uniform mat4 ShadowMapViewMatrix;                 /**< Transforms from world space to the light view space */
//layout (location = 3) uniform mat4 ShadowMapProjectionMatrix;           /**< Transforms from light view space to light clip space */
//layout (location = 4) uniform int UseSSAO;                              /**< Whether SSAO is enabled */
/* auto location */ uniform DirectionalLight Sun;                       /**< Directional light in the scene */

layout (binding = 0) uniform sampler2D ColorBuffer;
layout (binding = 1) uniform sampler2D ReflectionBuffer;
layout (binding = 2) uniform sampler2D NormalBuffer;
layout (binding = 3) uniform sampler2D DepthBuffer;
layout (binding = 4) uniform sampler2D EnvironmentMap;

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * Perspective division by the w component
 *
 * @param                       v 4D vector on which to perform perspective division
 *
 * @return 3D vector after perspective division
 */
vec3 wdiv(vec4 v) {
    return v.xyz / v.w;
}

/**
 * Computes lighting at a point from a directional light
 *
 * @param materialColor         Material color
 * @param directionalLight      Directional-light parameters
 * @param viewMatrix            Viewer view matrix
 * @param worldNormal           Point normal in world space
 * @param worldPosition         Point position in world space
 * @param attenuation           Light attenuation due to shadow mapping
 * @param ssao                  Screen-space ambient-occlusion value
 *
 * @return Lighting at the point
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
 * Inverse depth-range transformation
 *
 * @param depth                 Depth sampled from the depth buffer
 *
 * @return Depth in clip space
 */
float inverseDepthRangeTransformation(float depth) {
    return (2.0 * depth - gl_DepthRange.near - gl_DepthRange.far) /
            (gl_DepthRange.far - gl_DepthRange.near);
}

vec4 sampleEquirectangular(vec3 dir, sampler2D sampler, float level)  {
        vec2 uv;
        uv.x = atan(dir.z, dir.x);
        uv.y = acos(dir.y);
        uv /= vec2(2 * PI, PI);

        return textureLod(sampler, uv, level);
}



vec3 gBufferWorldPosition(vec2 bufferTexCoord) {
    float depthFromDepthBuffer = texture(DepthBuffer, bufferTexCoord).r;
    float depth = inverseDepthRangeTransformation(depthFromDepthBuffer);
    vec4 fragmentDcPosition = vec4(2*bufferTexCoord - vec2(1), depth, 1);
    vec4 fragmentWorldPosition4 = ViewerInverseViewProjectionMatrix*fragmentDcPosition;
    return wdiv(fragmentWorldPosition4);
}

vec4 shade(vec2 bufferTexCoord) {
    // Read the G-buffer
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

    vec3 fragmentWorldPosition = gBufferWorldPosition(bufferTexCoord);

    return shadeDirectionalLight(material,
                                 vec4(color, 1),
                                 Sun,
                                 ViewerViewMatrix,
                                 worldNormal,
                                 fragmentWorldPosition,
                                 1);
}


/**
 * Entry point for the fragment shader
 */
void main() {

    vec3 worldWaterNormal = normalize(fWorldNormal);

    vec4 cameraWorldPosition = inverse(ViewMatrix)*vec4(0, 0, 0, 1);

    vec3 v = normalize(wdiv(cameraWorldPosition) - wdiv(fWorldPosition));
    vec3 r = reflect(-v, worldWaterNormal);

    vec3 ndcPosition = fDcPosition.xyz / fDcPosition.w;
    vec2 bufferTexCoord = 0.5*ndcPosition.xy + vec2(0.5);

    float depthFromDepthBuffer = texture(DepthBuffer, bufferTexCoord).r;
    float depth = inverseDepthRangeTransformation(depthFromDepthBuffer);

    //vec4 groundColor = shade(bufferTexCoord);
    //vec3 groundWorldPosition = gBufferWorldPosition(bufferTexCoord);

    //vec4 debugColor = vec4(vec3(length(groundWorldPosition - wdiv(fWorldPosition))), 1);






    //float fresnel = dot(v, worldWaterNormal);
    float cosThetaV = max(0.0, dot(v, worldWaterNormal));
    float F0 = 0.05;
    float fresnel = F0 + (1 - F0)*pow(1 - cosThetaV, 5);

    float cosTheta = max(0, dot(worldWaterNormal, vec3(1, 1, 1)));

    vec3 diffuseColor = cosTheta*vec3(0.1, 0.2, 0.3);

    FragColor = vec4(mix(diffuseColor, sampleEquirectangular(r, EnvironmentMap, 0).rgb, fresnel), 0.8);
    //FragColor = 2*groundColor;
    //FragColor = debugColor;
    //FragColor = vec4(vec3(cosThetaV), 1);
}
