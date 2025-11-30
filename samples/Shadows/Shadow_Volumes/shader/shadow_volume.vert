#version 330

layout (location = 0) in vec3 Position;                                                                                 
layout (location = 1) in vec3 Normal;      
layout (location = 2) in vec2 TexCoord;      

out vec3 PosL;

void main()                                                                         
{                                                                                
    PosL = Position;
    // pass through a position so the pipeline has something defined; GS will replace it
    gl_Position = vec4(Position, 1.0);
}
