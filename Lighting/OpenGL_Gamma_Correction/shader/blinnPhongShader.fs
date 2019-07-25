#version 330 core
out vec4 FragColor;

// declare an interface block; see 'Advanced GLSL' for what these are.
in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;


// Texture
uniform sampler2D texture1;

//Light
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

//Eye
uniform vec3 viewPos;

//Switches
uniform int blinn;
uniform int gamma;

/*
 * Blinn-Phong Lightning Model
 */
vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightColor) {
	//diffuse
	vec3 lightDir = normalize(lightPos - fragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff*lightColor;

	// specular 
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);

	float spec = 0.0;

	if(blinn == 1){
		vec3 halfwayDir = normalize(lightDir + viewDir); //vector between LightDir Vec and View Vec 
		//specular shiniess exponent
		spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
		//  clamped dot product
	} else {
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
	}

	vec3 specular = spec * lightColor;
	// simple attenuation
	float max_distance = 1.5;
	float distance = length(lightPos - fragPos);
	float attentuation = 1.0 / (gamma==1 ? distance * distance : distance);
	// Without gamma correction => lin. function gives much more plausible results
	// with gamma correction => quad. func. (phys. correct!) gives much better result

	return diffuse+specular;
}


void main() 
{
	float GAMMA_VALUE = 2.2;

	vec3 color = texture(texture1, fs_in.TexCoords).rgb;
	vec3 lightning =  vec3(0.0);

	vec3 normal = normalize(fs_in.Normal);

	// Loop over all lights
	for(int i = 0; i < 4; ++i)
		lightning += BlinnPhong(normal, fs_in.FragPos, lightPositions[i], lightColors[i]);
	color *= lightning;

	// apply gamma  correction manually
	if(gamma==1)
		color = pow(color, vec3(1.0/GAMMA_VALUE)); // has to be applied to each fragment shader that contributes to the final output

	FragColor = vec4(color, 1.0);

}