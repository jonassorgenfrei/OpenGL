#version 330                                                                        
                                                                                    
layout (location = 0) in vec3 aPos;  

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};

uniform int layer;
uniform mat4 model;                                                                 
                                                                                                                                                                       
void main()                                                                         
{                                                                                   
    gl_Position = lightSpaceMatrices[layer] * model * vec4(aPos, 1.0);
}
