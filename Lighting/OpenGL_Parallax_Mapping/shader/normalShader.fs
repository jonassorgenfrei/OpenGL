#version 330 core

out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

in mat3 tbn;

uniform sampler2D diffuseMap;
/*
 * Normal vectors in normal map are expressed in tangent space 
 * (normals always point roughly in the positive z direction)
 */
uniform sampler2D normalMap;
uniform sampler2D depthMap;

uniform float height_scale;

uniform vec3 lightPos;


/*
 * ParallaxMapping
 *	@param - fragment's texture coordinates
 *	@param - fragment-to-view direction in Tangent Space
 *	@return displaced texture coordinates
 */
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir){
	vec2 currentTexCoords = texCoords;

	/* Steep Parallax Mapping */

	//	takes multiple samples to better pinpoint vector
	//		divides total depth range into multiple layers of the same height/depthMap
	//		for each of these layers we sample the depthmap shifting the tex coordinante along the dir P (until we find a sampled depth value that is below the d. value of the cur. layer)
	//		Problem: based on finite number of samples: getting: aliasing effects and clear distinctions between layers 

	const float minLayers = 8.0;
	const float maxLayers = 32.0;

	// looking straight onto a surface, there isn't much tex. displ. going on
	// from a an angle a lot more displacement
	// number of depth layers
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
			// dot Product of viewDir and pos z direction (== surface normal in tangent space) use result to align num. of samples 


	// calculate the size of each layer
	float layerDepth = 1.0 / numLayers;
	// depth of current layerDepth
	float currentLayerDepth = 0.0;
	// the amount to shift the texture coordinates per layer (from vector p)
	vec2 P = viewDir.xy * height_scale;
	vec2 deltaTexCoords = P / numLayers;

	// get initial values 
	float currentDepthMapValue = texture(depthMap, currentTexCoords).r;

	// loop over each depth layer and stop until we find the texture coord. offset along vec P that first returns a depth that's below the (displaced surface)
	while(currentLayerDepth < currentDepthMapValue) {
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = texture(depthMap, currentTexCoords).r;
		//get depth of next layer
		currentLayerDepth += layerDepth;
	} 
	// much more accuracy !

	/* ------------------------- */
	
	/* Reliefe Parallax Mapping */
	// most accurate result, BUT: more performance heavy
	/* ------------------------- */

	/* Parallax Occlusion Mapping */
	// more efficient, almost same result as Reliefe Parallax Mapping 
	// based on same principles as steep parallax Mapping
	//	instead of taking 1. coord after collision 
	//		-> lin. interpolate on how far the surface height is from the depth layer's calue of both layers

	// get texture coordinates before collsion (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for lin. interpolation
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
	currentTexCoords = finalTexCoords;
	/* ------------------------- */


	/* Parallax Mapping 1 Sample */
	
	/* 
	float height = texture(depthMap, texCoords).r;	// sample from depth Map the current fragment H(A)
	vec2 p = viewDir.xy / viewDir.z * (height * height_scale); 
	// calculate P as the x and y component of the tangent-space viewDir vec devided by its y component and scale it by the fragment's height
	// division viewDir.xy / viewDir.z: viewDir -> normal. .z = 0.0 .. 1.0 | when viewDir almost parallel .z ~ 0
	// height_scale uniform for extra control as the parallax effect
	// WITHOUT Z DIVISION: Parallax Mapping with Offset Limiting
	currentTexCoords = texCoords - p; */

	/* ------------------------- */

	return currentTexCoords; // subtract vec p from the texture coord. to get fin. displaced texture coord.
}

/*
 * Blinn-Phong Lightning Model
 */
vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPosition, vec3 lightColor, vec3 viewPos) {
	//diffuse
	vec3 lightDir = normalize(lightPosition - fragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff*lightColor;

	// specular 
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	
	float spec = 0.0;

	vec3 halfwayDir = normalize(lightDir + viewDir); //vector between LightDir Vec and View Vec 
	//specular shiniess exponent
	spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	//  clamped dot product
	
	vec3 specular = spec * vec3(0.2);
	// simple attenuation
	float max_distance = 1.5;
	float distance = length(lightPosition - fragPos);
	float attentuation = 1.0 / distance;
	// Without gamma correction => lin. function gives much more plausible results

	return diffuse+specular;
}

/* 
 * Using TBN-Matrix (2 ways to use it) 
 *	1) giving TBN Matrix to fs and using it to transform normals (Tangent Space -> world space)
 *	2) taking inverse of TBN and transform any vector from world to tangent space (world space -> tangent space)
 *		Advantage: all other vectors can be transformed in the vertex Shader (vs runs less often then fs)
 */
void main() 
{
	// offset texture coordinates with Parallax Mapping
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec2 texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
	
	// discard fragment whenever it samples outside the def. coord Range
	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
		discard; // !!!! doesn't work properly on all types of surfaces 

	// sample textures with new texture coords
	vec3 color = texture(diffuseMap, texCoords).rgb;

	// obtain normal from normal map in range [0,1]
	vec3 normal = texture(normalMap, texCoords).rgb;

    // ambient
    vec3 ambient = 0.1 * color;

	// transform normal vector to range [-1, 1]
	normal = normalize(normal * 2.0 - 1.0);

	vec3 blinnPhong = BlinnPhong(normal, fs_in.FragPos, lightPos, color, fs_in.TangentViewPos);
	
	FragColor = vec4(ambient + blinnPhong, 1.0);
	FragColor = vec4(tbn*normal,1);
}