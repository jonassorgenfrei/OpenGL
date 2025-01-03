#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 Pos;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 eyePos;

vec3 coolColor = vec3(0, 0, 0.55);
vec3 warmColor = vec3(0.3, 0.3, 0);

vec3 lit(vec3 l, vec3 n, vec3 v) {
	vec3 r_l = reflect(-l, n);
	float s  = clamp(100.0 * dot(r_l, v) - 97.0, 0.0, 1.0);
	vec3 highlightColor  = vec3(2,2,2);
	return mix(warmColor, highlightColor, s);
};

void main()
{
	vec3 n = normalize(Normal);
	vec3 v = normalize(eyePos-Pos);
	vec3 l = normalize(lightPos-Pos);
	float NdL = clamp(dot(n, l), 0.0, 1.0);

	FragColor = vec4(0.5*coolColor + NdL * lit(l,n,v), 1.0);
}