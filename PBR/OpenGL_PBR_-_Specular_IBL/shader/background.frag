
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;
uniform float mipmap;
uniform samplerCube environmentMap;

void main()
{	
	// sample environment Map using interpolated cube positions
	// last parameter set Mipmap level
	vec3 envColor = textureLod(environmentMap, WorldPos, mipmap).rgb;

    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));	// tone map
    envColor = pow(envColor, vec3(1.0/2.2));	// gamma correct
    
    FragColor = vec4(envColor, 1.0);
}