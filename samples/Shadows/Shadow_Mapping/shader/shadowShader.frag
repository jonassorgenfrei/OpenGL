#version 330 core
out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;			// world-space vertex position
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace; // world-space vertex position transformed to light space
} fs_in;

const float BIAS = 0.005;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float shadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	// Perspective Devide
	// transform clip-space coordinates in the range [-w,w] to [-1, 1]
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform NDC coordinantes to the range [0,1]
	projCoords = projCoords * 0.5 +0.5;
	// Sample the depth map
	float closestDepth = texture(shadowMap, projCoords.xy).r;	//from lights point of view
	// get current depth at the fragment 
	float currentDepth = projCoords.z;
	
	// check if currentDepth is highger than closestDepth
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	// force shadow when z coordinante is larger than 1.0
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}


float shadowCalculationBiased(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	// Perspective Devide
	// transform clip-space coordinates in the range [-w,w] to [-1, 1]
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform NDC coordinantes to the range [0,1]
	projCoords = projCoords * 0.5 +0.5;
	// Sample the depth map
	float closestDepth = texture(shadowMap, projCoords.xy).r;	//from lights point of view
	// get current depth at the fragment 
	float currentDepth = projCoords.z;
	//Shadow Bias (offsets the depth of the surface [or  the shadow map] by a small bias amount)
	//float bias = BIAS;
	float bias = max(0.05 * (1.0 - dot(normal,lightDir)), BIAS); // change amount of bias based on the surface angle towards the light: something we can solve with the dot product
	
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// force shadow when z coordinante is larger than 1.0
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}

float shadowCalculationPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	// Perspective Devide
	// transform clip-space coordinates in the range [-w,w] to [-1, 1]
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform NDC coordinantes to the range [0,1]
	projCoords = projCoords * 0.5 +0.5;
	// Sample the depth map
	float closestDepth = texture(shadowMap, projCoords.xy).r;	//from lights point of view
	// get current depth at the fragment 
	float currentDepth = projCoords.z;
	//Shadow Bias (offsets the depth of the surface [or  the shadow map] by a small bias amount)
	//float bias = BIAS;
	float bias = max(0.05 * (1.0 - dot(normal,lightDir)), BIAS); // change amount of bias based on the surface angle towards the light: something we can solve with the dot product
	bias = 0;
	// PCF (percentage-closer filtering) to produce softer shadows
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);	// width and height of texture at mipmap level 0
	for(int x = -1; x <= 1; ++x) // sample 3 cols
	{
		for(int y = -1; y <= 1; ++y) // sample 3 rows
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0; // ave. result by total samples taken

	// force shadow when z coordinante is larger than 1.0
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}

void main() 
{
	vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(0.3);
	//ambient
	vec3 ambient = 0.3 * color;
	 // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
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
    //float shadow = shadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);       
	float shadow = shadowCalculationBiased(fs_in.FragPosLightSpace, normal, lightDir);      
	//float shadow = shadowCalculationPCF(fs_in.FragPosLightSpace, normal, lightDir);      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
} 

