
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;	// sample environment Map using interpolated cube positions
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));	// tone map
    envColor = pow(envColor, vec3(1.0/2.2));	// gamma correct
    
    FragColor = vec4(envColor, 1.0);
}