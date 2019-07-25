#version 330 core
out vec4 FragColor;

// declare an interface block; see 'Advanced GLSL' for what these are.
in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs_in;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform int blinn;

void main() 
{
	vec3 color = texture(texture1, fs_in.TexCoords).rgb;
	//ambient 
	vec3 ambient = 0.05 * color;
	//diffuse
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	vec3 normal = normalize(fs_in.Normal);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff*color;
		
	//specular 
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	
	float spec = 0.0;

	if(blinn == 1){
		vec3 halfwayDir = normalize(lightDir + viewDir); //vector between LightDir Vec and View Vec 
		//specular shiniess exponent
		spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
		// set shiniess exponent between 2 and 4 times the Phong shininess (angle between vecs is smaller!!)
		//  clamped dot product
	} else {
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
	}

	vec3 specular = vec3(0.3) * spec; 
	// assuming bright white LIGHT COLOR => vec3(0.3)

	FragColor = vec4(ambient + diffuse + specular, 1.0);

}