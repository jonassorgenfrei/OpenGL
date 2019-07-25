#version 330 core

out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;

/*
 * Normal vectors in normal map are expressed in tangent space 
 * (normals always point roughly in the positive z direction)
 */
uniform sampler2D normalMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

/*
 * Blinn-Phong Lightning Model
 */
vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPosition, vec3 lightColor) {
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
	// get diffuse color
    vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
	
	// obtain normal from normal map in range [0,1]
	vec3 normal = texture(normalMap, fs_in.TexCoords).rgb;
	
	// transform normal vector to range [-1, 1]
	normal = normalize(normal * 2.0 - 1.0);

	vec3 blinnPhong = BlinnPhong(normal, fs_in.FragPos, lightPos, color);
	
	FragColor = vec4(ambient + blinnPhong, 1.0);

}