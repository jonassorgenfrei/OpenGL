#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
	//normalized device coordinates will then always have a z value to 1.0 
	//w/w = 1.0 
	//skybox will rendered whereever there are no objects visible (only then it will pass the depth test
}  