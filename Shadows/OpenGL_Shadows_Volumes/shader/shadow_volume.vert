#version 330

layout (location = 0) in vec3 Position;                                                                                 
layout (location = 1) in vec3 Normal;      
layout (location = 2) in vec2 TexCoord;      

out vec3 PosL;
                                                                                    
void main()                                                                         
{                     
    // forward position as is
    PosL = Position;
}
