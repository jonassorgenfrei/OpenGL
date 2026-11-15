#version 330 core

out vec4 FragColor;

struct BaseLight
{
    vec3 Color;
    float AmbientIntensity;
    float DiffuseIntensity;
};

struct DirectionalLight
{
    BaseLight Base;
    vec3 Direction;
};

// G-buffer Input
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform DirectionalLight gDirectionalLight;
uniform vec3 viewPos;
uniform float gSpecularPower;

uniform vec2 gScreenSize;

vec4 CalcLightInternal(BaseLight Light,
					   vec3 LightDirection,
					   vec3 WorldPos,
					   vec3 Normal,
					   vec3 MaterialColor,
					   float MaterialSpecularIntensity)
{
	// Albedo affects ambient and diffuse reflection, while the packed
	// G-buffer specular channel controls only the specular highlight.
	vec4 AmbientColor = vec4(Light.Color * MaterialColor * Light.AmbientIntensity, 1.0);
    float DiffuseFactor = dot(Normal, -LightDirection);

    vec4 DiffuseColor  = vec4(0, 0, 0, 0);
    vec4 SpecularColor = vec4(0, 0, 0, 0);

    if (DiffuseFactor > 0.0) {
		DiffuseColor = vec4(Light.Color * MaterialColor * Light.DiffuseIntensity * DiffuseFactor, 1.0);

        vec3 VertexToEye = normalize(viewPos - WorldPos);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);        
        if (SpecularFactor > 0.0) {
            SpecularFactor = pow(SpecularFactor, gSpecularPower);
			SpecularColor = vec4(Light.Color * MaterialSpecularIntensity * SpecularFactor, 1.0);
        }
    }

    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcDirectionalLight(vec3 WorldPos, vec3 Normal, vec3 MaterialColor, float MaterialSpecularIntensity)
{
    return CalcLightInternal(gDirectionalLight.Base,
							 gDirectionalLight.Direction,
							 WorldPos,
							 Normal,
							 MaterialColor,
							 MaterialSpecularIntensity);
}

vec2 CalcTexCoord()
{
	/*
	 * Build in gl_FragCoord:
	 *	X - screen X coord
	 *	Y - screen Y coord
	 *  Z - depth component
	 *  W - 1/W component
	 */
   return gl_FragCoord.xy / gScreenSize;
} 

void main() {
	vec2 TexCoord = CalcTexCoord();
	vec3 WorldPos = texture(gPosition, TexCoord).xyz;
    vec3 Color = texture(gAlbedoSpec, TexCoord).xyz;
    vec3 Normal = texture(gNormal, TexCoord).xyz;
	float Specular = texture(gAlbedoSpec, TexCoord).a;
    Normal = normalize(Normal);

	FragColor = CalcDirectionalLight(WorldPos, Normal, Color, Specular);
}
