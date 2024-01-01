#version 430 core
out vec4 FragColor;

const int NUM_CASCADES = 4;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 LightSpacePos[NUM_CASCADES];
} fs_in;

uniform sampler2D diffuseTexture;
// use 2d array textures
uniform sampler2D ShadowMap[NUM_CASCADES];

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float farPlane;

uniform mat4 view;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

int getCascadeLayer(float depthValue) {   
    // alternative 
    // ClipSpacePosZ <= cascadePlaneDistances[i] 
    // ClipSpacePosZ from vertex shader gl_Position.z

    // logic to decide which shadow layer to sample
    int layer = -1;

#ifdef USE_LOOP
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }
#else
    // branch avoidance techniques
    // Where csmClipSpaceZFar is a vec4 containing the cascadePlaneDistances
    vec4 csmClipSpaceZFar = vec4(cascadePlaneDistances[0], cascadePlaneDistances[1], cascadePlaneDistances[2], cascadePlaneDistances[3]);   // NOTE: this works only with 4 layers!
    vec4 res = step(csmClipSpaceZFar, vec4(depthValue));

    layer = int(res.x + res.y + res.z + res.w);
#endif
    return layer;
}

vec4 colorMapSample(vec3 fragPosWorldSpace){
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
    int layer = getCascadeLayer(depthValue);
    
    vec4 colors[] = {vec4(0.1,0,0,0), vec4(0,0.1,0,0),vec4(0,0,0.1,0),vec4(0.1,0.1,0,0)};

    return colors[layer];
}

float ShadowCalculation(vec3 fragPosWorldSpace)
{
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
   
    int layer = getCascadeLayer(depthValue);

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope) to prevent shadow acne
    vec3 normal = normalize(fs_in.Normal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 0.5f;
    // reducing the bias, as the cascades get further away from the view point
    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;

    vec2 texelSizeMap1 = 1.0 / vec2(textureSize(ShadowMap[0], 0));
    vec2 texelSizeMap2 = 1.0 / vec2(textureSize(ShadowMap[1], 0));
    vec2 texelSizeMap3 = 1.0 / vec2(textureSize(ShadowMap[2], 0));
    vec2 texelSizeMap4 = 1.0 / vec2(textureSize(ShadowMap[3], 0));

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            // sample from 2d array texture, 3rd dimension parameters selects the layer
            float pcfDepth;
            switch(layer) {
	            case 0: pcfDepth = texture(ShadowMap[0], projCoords.xy + vec2(x, y) * texelSizeMap1).x; break;
	            case 1: pcfDepth = texture(ShadowMap[1], projCoords.xy + vec2(x, y) * texelSizeMap2).x; break;
	            case 2: pcfDepth = texture(ShadowMap[2], projCoords.xy + vec2(x, y) * texelSizeMap3).x; break;
                case 3: pcfDepth = texture(ShadowMap[3], projCoords.xy + vec2(x, y) * texelSizeMap4).x; break;
	            default: pcfDepth = 1.0;
	        }
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
     
    return shadow;
}


void main()
{          
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * color;
    // diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    
    // calculate shadow                 

    float shadow = ShadowCalculation(fs_in.FragPos);   

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
    // DEBUG color fb color with sampled map color
    FragColor += colorMapSample(fs_in.FragPos);
    //FragColor = vec4(vec3(1,0,0)*(1-ShadowFactor), 1.0);
}
