#version 330                                                                        
                                                                                    
layout (location = 0) in vec3 Position;                                             
layout (location = 1) in vec3 Normal;                                             
layout (location = 2) in vec2 TexCoord;                                               
                                                                                    
uniform mat4 model;                                                                  
uniform mat4 view;      
uniform mat4 projection; 

uniform bool invertedNormals = false;
                                                                                    
out vec2 TexCoord0;                                                                 
out vec3 Normal0;                                                                   
out vec3 WorldPos0;                                                                 
                                                                                    
void main()                                                                         
{          
	mat4 gWVP = projection * view * model;
    gl_Position = gWVP * vec4(Position, 1.0);                                       
    TexCoord0   = TexCoord;                                                         
    Normal0     = (model * vec4(Normal, 0.0)).xyz;      
	Normal0		= (invertedNormals ? -Normal0 : Normal0);
    WorldPos0   = (model * vec4(Position, 1.0)).xyz;                               
}