#version 430 core

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec3 fWorldNormal;                       
in vec4 fWorldPosition;                     
in vec2 fTexCoord;                         

out vec4 FragColor;                         

// ----------------------------------------------------------------------------
//
// Types
//
// ----------------------------------------------------------------------------

struct Material {
	//Textures
	sampler2D texture_height1;
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
	sampler2D texture_emission1;
    float shininess;
}; 

 /**
  * Isotropic Pointlight
  * Pointlight data is stored in a buffer object
  * NOTE: consider Memory Layout
  */
struct PointLight {
    vec3 position;
    float radius;
    vec4 color;
};

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 ViewMatrix;

//Materials
uniform Material material;

// number of light sources
uniform int NumLights;
// size of tiles vec2(width, height)
uniform ivec2 TileSize;
// amount tiles vec2(x, y)
uniform ivec2 NumTiles;
// flag if heatmap should be overlayed
uniform bool RenderHeatmap;

/**
 * Light Data buffer
 * contains data of lights 
 */
layout (std430, binding = 0) buffer LightsBuffer {
    PointLight lights[];
};

/**
 * @brief Buffer with indices of visiblie lights
 *
 * Buffer includes for every tile the indices of the lights overlapping the
 * truncated pyramid
 *
 * Layout:
 *
 * Tile 1                  Tile 2
 * |                         |
 * v                         v
 * +---+---+---+---+-----+---+-------
 * | N | 0 | 1 | 2 | ... | K | ...
 * +---+---+---+---+-----+---+-------
 *
 * N - amount of light sources
 * [0...(K)] - indicies of the lights 
 * The Capacity (K) is equal for each Tile and is the number of overall light sources.
 * This avoids the need for an atomic counter to reserve a unique memory-amount 
 * for each tile.
 * This avoids the need for Indice pointers in a seperate texture/buffer.
 *
 * [0] Since GLSL doesn't have pointer types, pointer structures are implemented as indices into 
 * array elements (buffers).
 */
layout (std430, binding = 1) buffer VisibleLightIndicesBuffer {
    int visibleLightIndices[];
};

// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------

/**
 * @brief W-Division
 *
 * Calculates w-division for a 4d vector
 *
 * @param v      input vector
 *
 * @return vec3 with w-division applied
 */
vec3 wdiv(vec4 v) {
    return v.xyz / v.w;
}

/**
 * @brief Calculate camera position in world space
 *
 * Calculates the camera position, by transform the origin of the view coordinate system
 * to the world coordinate system using the ivnerse view matrix 
 *
 * @param viewMatrix    ViewMatrix
 *
 * @return position of the camera in world space
 */
vec3 makeCameraWorldPosition(mat4 viewMatrix) {
    return wdiv(inverse(viewMatrix)*vec4(0, 0, 0, 1));
}

/**
 * @brief BRDF to calculate lighting on a surface point
 *
 * Using a simplified Phong-Modell.
 *
 * @param material              Material from model
 * @param pointLight            Pointlight attributes
 * @param baseColor             Diffuse color of surface (simplified Phong)
 * @param cameraWorldPosition   position of the camera in world space
 * @param worldPosition         position of the surface point in worldspace
 * @param worldNormal           normale of the surface point in worldspace
 *
 * @return Lighting of the surface point
 */
vec4 shadePointLight(Material material,
                     PointLight pointLight,
                     vec4 baseColor,
                     vec3 cameraWorldPosition,
                     vec3 worldPosition,
                     vec3 worldNormal)
{
    vec3 cameraDirection = normalize(cameraWorldPosition - worldPosition);
    vec3 lightDirection = normalize(pointLight.position - worldPosition);
    vec3 reflectionDirection = reflect(-lightDirection, worldNormal);

    vec4 i_amb  = baseColor*pointLight.color;
    vec4 i_diff = max(0, dot(worldNormal, lightDirection)) * baseColor* pointLight.color;
    vec4 i_spec = pow(max(0, dot(reflectionDirection, cameraDirection)), material.shininess) * pointLight.color;

    float distance = length(pointLight.position - worldPosition);
    // simple (but unrealistic) fall-off modell for the light
    float d = max(0, 1 - (1.0/pointLight.radius)*distance);

    return d*(i_amb + i_diff + i_spec);
}

/**
 * @brief Calculate the color for the heatmap
 *
 * @param value     Values between [0,1] to calculate heatmap
 *
 * @return color of the heatmap for the given value
 */
vec4 heatmap(float value) {
    vec3 color = vec3(0);

    // color values of the heatmap
    const vec3 color0 = vec3(0, 0, 1);
    const vec3 color1 = vec3(0, 1, 0);
    const vec3 color2 = vec3(1, 1, 0);
    const vec3 color3 = vec3(1, 0, 0);
    const vec3 color4 = vec3(1, 0, 1);

    if (value >= 0 && value < 0.25) {
        color = mix(color0, color1, value*4);
    } else if (value >= 0.25 && value < 0.5) {
        color = mix(color1, color2, (value - 0.25)*4);
    } else if (value >= 0.5  && value < 0.75) {
        color = mix(color2, color3, (value - 0.5)*4);
    } else if (value >= 0.75 && value < 1.0) {
        color = mix(color3, color4, (value - 0.75)*4);
    }
    return vec4(color, 1);
}

void main() {
    // read texture
    vec4 materialColor = texture(material.texture_diffuse1, fTexCoord); 

    // remove alpha 
    if(materialColor.a < 0.1)
        discard;

    // calculate position of the light indicies in the buffer object
    ivec2 tilePosition = ivec2(gl_FragCoord.xy) / TileSize.xy;
    uint linearWorkGroupIndex = uint(NumTiles.x*tilePosition.y + tilePosition.x);

    // amount of lights are stored in the buffer as first component
    int numLights = visibleLightIndices[(NumLights + 1)*linearWorkGroupIndex];

    // calculate the contribution of each light to the global lighting of the surface pointLight
    // accumulate the intensity
    vec4 color = vec4(0, 0, 0, 1);    
    for (int lightIndex = 0; lightIndex < numLights; ++lightIndex) {
        int index = visibleLightIndices[(NumLights + 1)*linearWorkGroupIndex + lightIndex + 1];
        PointLight light = lights[index];

        vec4 illumination = shadePointLight(material,
                                            light,
                                            materialColor,
                                            makeCameraWorldPosition(ViewMatrix),
                                            wdiv(fWorldPosition),
                                            fWorldNormal);
        color += illumination;
    }

    // add some minimal ambient color
    FragColor = color + materialColor*0.2;

    // layer heatmap semi-transparent on top of the scene.
    if (RenderHeatmap) {
      FragColor = mix(FragColor, heatmap(float(numLights)/NumLights), 0.7);
    }
}
