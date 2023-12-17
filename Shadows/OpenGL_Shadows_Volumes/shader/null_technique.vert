#version 330

layout (location = 0) in vec3 Position; 

uniform mat4 projection;
uniform mat4 view;

void main()
{          
    gl_Position = projection * view * vec4(Position, 1.0);
}
