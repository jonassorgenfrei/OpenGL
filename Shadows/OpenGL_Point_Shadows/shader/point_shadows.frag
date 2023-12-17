#version 330 core
out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap; // Shadow Map (Cube)

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;
uniform bool shadows;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

/**
 * NO PCF
 * @param fragPos - Fragment Position
 * @return 1.0 when the fragment is in the shadow
 *		   0.0 when the fragment is not in the shadow
 */
float ShadowCalculation(vec3 fragPos)
{
	// retrieve depth of the cubemap

	// vector between frag. position & light position
	vec3 fragToLight = fragPos - lightPos; // direction vector to sample the cubemap
	// doesnt need to be a unit vector
	
	// retrieve the depth value between the current fragment and the light source which we can easily 
	// obtain by taking the length of fragToLight due to how we calculated depth values in the cubemap
	float currentDepth = length(fragToLight);

	// sample from the depth map, using the fragToLight vector 
	float closestDepth = texture(depthMap, fragToLight).r; // normalized depth value between the 
	// light source and its closest visible fragment

	// transform from [0,1] to [0, far_plane] 
	closestDepth *= far_plane;

	// compare both depth values to see which is closer than the other and determine whether the current 
	// fragment is in ShadowCalculation

	// include Bias to avoid acne effect
	float bias = 0.05; // use a much larger bias since depth is now in [near_plane, far_plane] range

	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

/**
 * NO PCF
 * @param fragPos - Fragment Position
 * @return 1.0 when the fragment is in the shadow
 *		   0.0 when the fragment is not in the shadow
 */
float ShadowCalculationPCF1(vec3 fragPos)
{
	// retrieve depth of the cubemap

	// vector between frag. position & light position
	vec3 fragToLight = fragPos - lightPos; // direction vector to sample the cubemap
	// doesnt need to be a unit vector
	
	
	// retrieve the depth value between the current fragment and the light source which we can easily 
	// obtain by taking the length of fragToLight due to how we calculated depth values in the cubemap
	float currentDepth = length(fragToLight);

	float shadow = 0.0;
	float bias = 0.05;
	float samples = 4.0; // 64 samples each fragment: thats a lot!
	// most samples are redudant to the original direction vector
	float offset = 0.1;
	for(float x = -offset; x < offset; x += offset / (samples * 0.5) ) 
	{
		for (float y = -offset; y < offset; y += offset / (samples * 0.5)) 
		{
			for(float z = -offset; z < offset; z += offset / (samples * 0.5))  // adding the 3d dimension
			{
				float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r;
				closestDepth *= far_plane; // remaping [0,1] -> [0,far_plane]
				if(currentDepth - bias > closestDepth)
					shadow += 1.0;
			}
		}
	}
	shadow /= (samples * samples * samples); // average samples

	return shadow;
}

/**
 * @param fragPos - Fragment Position
 * @return 1.0 when the fragment is in the shadow
 *		   0.0 when the fragment is not in the shadow
 */
float ShadowCalculationPCF2(vec3 fragPos)
{
	// retrieve depth of the cubemap

	// vector between frag. position & light position
	vec3 fragToLight = fragPos - lightPos; // direction vector to sample the cubemap
	// doesnt need to be a unit vector
	
	
	// retrieve the depth value between the current fragment and the light source which we can easily 
	// obtain by taking the length of fragToLight due to how we calculated depth values in the cubemap
	float currentDepth = length(fragToLight);

	// ----- PCF 2
		
	float shadow = 0.0;
	float bias = 0.15; // based on the context!
	int samples = 20;
	float viewDistance = length(viewPos - fragPos);
		
	//float diskRadius = 0.05; // offset to a specific diskRadius

	// change diskRadius based on how far the viewer is away from a fragment
	// making the shadows softer when far away and sharper when close by
	float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;

	for(int i = 0; i < samples; ++i)
	{
		/**
			* only sample in perpendicuar directions of the sample direction vector
			* we take an array of offset directions that are all roughly seperable
			* e.G. each of them point in completely different directions 
			* reducing the number of sub-directions that are close together
			*/
		float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
		closestDepth *= far_plane;	// remaping [0,1] -> [0,far_plane]
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);
	// ---------

	return shadow;
}


/**
 * @param fragPos - Fragment Position
 * @return 1.0 when the fragment is in the shadow
 *		   0.0 when the fragment is not in the shadow
 */
vec4 VisSadowMap(vec3 fragPos)
{
	// retrieve depth of the cubemap

	// vector between frag. position & light position
	vec3 fragToLight = fragPos - lightPos; // direction vector to sample the cubemap
	// doesnt need to be a unit vector
	
	// retrieve the depth value between the current fragment and the light source which we can easily 
	// obtain by taking the length of fragToLight due to how we calculated depth values in the cubemap
	float currentDepth = length(fragToLight);

	// sample from the depth map, using the fragToLight vector 
	float closestDepth = texture(depthMap, fragToLight).r; // normalized depth value between the 
	// light source and its closest visible fragment

	// transform from [0,1] to [0, far_plane] 
	closestDepth *= far_plane;

	// DEBUG: display closestDepth (to visualize depth cubemap)
    return vec4(vec3(closestDepth / far_plane), 1.0);   
}

void main() 
{
	vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;//
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(0.3);
	// ambient 
	vec3 ambient = 0.3 * color;
	// diffuse
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;
	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 refelectDir = reflect(-lightDir, normal);
	float spec = 0.0;
	vec3 halfwayDir = normalize(lightDir + viewDir);
	spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
	vec3 specular = spec * lightColor;
	// calcuate shadow
	
	float shadow = shadows ? ShadowCalculation(fs_in.FragPos) : 0.0;  // calc shadow factor 
	//float shadow = shadows ? ShadowCalculationPCF1(fs_in.FragPos) : 0.0;  // calc shadow factor 
	//float shadow = shadows ? ShadowCalculationPCF2(fs_in.FragPos) : 0.0;  // calc shadow factor 

	vec3 lighting = (ambient + (1.0-shadow) * (diffuse + specular))*color;
	// influence the lighting's diffuse & specular component by the shadow factor

	FragColor = vec4(lighting, 1.0);

	// DEBUG SHADOW MAP
	//FragColor = VisSadowMap(fs_in.FragPos);
}