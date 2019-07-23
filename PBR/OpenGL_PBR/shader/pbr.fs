#version 330 core
/* out */
out vec4 fragColor;

/* const */
const float PI = 3.14159265359;

/* In Variables from Vertex Shader */
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

/* Material Properties */
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

/* Light */
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

/* Camera Position */
uniform vec3 camPos;

/**
 * Fresnel Schlick 
 */
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

/**
 * GGX (Normal) Distribution 
 */
 float DistributionGGX(vec3 N, vec3 H, float roughness)
 {
	// squaring roughness: lighting looks more correct 
	float a = roughness*roughness;
	float a2 = a*a;
	float NdotH = max(dot(N,H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
 }

 /**
  * Geometry Schlick GGX Function 
  */
float GeometrySchlickGGX(float NdotV, float roughness) 
{
	// squaring roughness: lighting looks more correct 
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

/**
 * Geometry Smith Function 
 */
 float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
 {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
 }

void main() {

	vec3 N = normalize(Normal);				// Fragment Normal
	vec3 V = normalize(camPos - WorldPos);	// View Direction 

		
	// Fresnel equation 
	// ================
	vec3 F0 = vec3(0.04);	// surface reflection at zero incidence
							// how much surface reflects if looking directly
							// at the surface
	// in PBR metalllic workflow, assumption:
	// most dielectric surfaces look visually correct 
	// with a constant F0 of 0.04

	// interpolating F0 between original F0 and albedo val given the metallic
	// property
	F0 = mix(F0, albedo, metallic); //  specify F0 for metallic surfaces as 
									// then given by the albedo value

	vec3 Lo = vec3(0.0);	// 
	// 4 point lights loop
	for(int i = 0; i < 4; ++i) {
		vec3 L = normalize(lightPositions[i] - WorldPos); // Light Direction
		vec3 H = normalize(V+L); // view + light direction

		// calculate light's individual radiance 
		float distance = length(lightPositions[i] - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		// Cook-Torrance specular BRDF Term
		// ================================
		
			// Fresnel equation 
			// ================
			// calc ratio between specular and diffuse reflection
			// (how much surface reflects light versus how much it refracts light)
			vec3 F = fresnelSchlick(max(dot(H,V), 0.0), F0);

			// Distribution Function D
			// =======================
			float NDF = DistributionGGX(N, H, roughness); // Normal distribution 
			// NOTE: pass roughness directly 

			// Geometry Function G	
			// ===================
			float G = GeometrySmith(N, V, L, roughness);
			// NOTE: pass roughness directly 
		
		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
		vec3 specular = numerator / max(denominator, 0.001);
		// constrain denominator: preventing devide by zero

		// incident angle
		// ==============
		float NdotL = max(dot(N, L), 0.0);

		// sum light's individual radiance scaled by BRDF & incident angle
		// ===============================================================
		vec3 kS = F;	// Fresnel Value,  energy of light that gets reflected
		vec3 kD = vec3(1.0) - kS;	// light energy that gets regracted

		kD *= 1.0 - metallic; // metallic surfaces don't refract light
							  // --> no diffuse reflections
		
		Lo += (kD * albedo / PI + specular) * radiance * NdotL; // outgoing radiance
		// effectively the result of the reflactance equation's integral
	}

	// add (improvised) ambient term
	vec3 ambient = vec3(0.03) * albedo  * ao;
	vec3 color = ambient + Lo;

	// gamma correction, because lighting in linear space (IMPORTANT!)
	// (more physical correct) inverse-square law
	// [less physically correct, but more control light's energy falloff: 
	//	constant, linear, quadratic attenuation	]
	color = color / (color + vec3(1.0)); // tone mapping; Reinhard operator
	color = pow(color, vec3(1.0/2.2));	// gamma correct

	fragColor = vec4(color, 1.0);
}
