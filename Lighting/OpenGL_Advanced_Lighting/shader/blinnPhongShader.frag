#version 330 core
out vec4 FragColor;

#define PI 3.14159265f

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
uniform int orenNayar;
uniform float diffuseRoughness;

vec3 OrenNayarDiffuse(vec3 viewDir, vec3 lightDir, vec3 normal, float roughness, vec3 lightColor)
{
	// based on https://garykeen27.wixsite.com/portfolio/oren-nayar-shading
	// aims to simulate the way light spreads across a rough
	// matte surface more accurately than the simple Lambertian method
	// takes into account; 
	//   roughness of surface
	//   incidence angle of light

    float NdotL  = max(0.0, dot(normal, lightDir));
    float NdotV = max(0.0, dot(normal, viewDir));

    // Calculate the angles between the light/view directions and the normal
    float angleVN = acos(NdotV);
    float angleLN = acos(NdotL);

	// A and B are precomputed coefficients based on the surface roughness
    float alpha = max(angleVN, angleLN);
    float beta = min(angleVN, angleLN);
    float gamma = cos(angleVN - angleLN);
 
    float roughness2 = roughness * roughness;
 
    float A = 1.0 - 0.5 * (roughness2 / (roughness2 + 0.57));
    float B = 0.45 * (roughness2 / (roughness2 + 0.09));
    float C = sin(alpha) * tan(beta);
 
    vec3 diff = lightColor * (NdotL * (A + ((B * max(0.0, gamma)) * C)));

    return diff;
}

vec3 OrenNayarDiffuseVar2(vec3 viewDir, vec3 lightDir, vec3 normal, float roughness, vec3 lightColor)
{	
	// based on: 
	// https://github.com/ranjak/opengl-tutorial/blob/master/shaders/illumination/diramb_orennayar_pcn.vert
	// aims to simulate the way light spreads across a rough
	// matte surface more accurately than the simple Lambertian method
	// takes into account; 
	//   roughness of surface
	//   incidence angle of light

	// Z = zenith angle, A = azimuth angle
	// Angle from surface to light
	float cosZi = dot(normal, lightDir);

	// Light coming from behind -> reflectance = 0
	if (cosZi <= 0.0f) {
	return vec3(0);
	}
	// Angle from surface to camera
	float cosZr = dot(normal, viewDir);

	// Using vector math, the formula can be reduced down to
	// using only 3 dot products (no sine, sqrt, etc.)
	float sigma2 = roughness*roughness;
	float termA = 1.0f - 0.5f * sigma2 / (sigma2 + 0.57f);

	float termB = 0.45f * sigma2 / (sigma2 + 0.09f);
	float cosAzimuthSinaTanb = (dot(-lightDir, viewDir) - cosZr * cosZi) / max(cosZr, cosZi);

	vec3 diff = cosZi * (termA + termB * max(0.0f, cosAzimuthSinaTanb)) * lightColor;

    return diff;
}

void main() 
{
	vec3 color = texture(texture1, fs_in.TexCoords).rgb;
	//ambient 
	vec3 ambient = 0.05 * color;
	//diffuse
	vec3 diffuse = vec3(0.0f);

	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	vec3 normal = normalize(fs_in.Normal);
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);

	if(orenNayar == 1) {
		diffuse = OrenNayarDiffuseVar2(viewDir, lightDir, normal, diffuseRoughness, vec3(1,1,1))*color;
	} else {
		float diff = max(dot(lightDir, normal), 0.0);
		diffuse = diff*color;
	}

	//specular 
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