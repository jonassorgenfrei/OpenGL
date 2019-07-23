#version 330 core
out vec4 FragColor;

in vec3 ourColor;
uniform vec4 ourColor2; // we set this variable in the OpenGL code.

void main()
{
    //FragColor = vec4(ourColor, 1.0f);
	FragColor = ourColor2;
}