#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{             
		//Just a color for boarder the object
        FragColor = vec4(color, 1.0);
}