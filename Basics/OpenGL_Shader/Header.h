//Fragment Shader
#version 330 core
out vec4 FragColor;
in vec4 vertexColor; //the input variable from the vertex shader (same name & same type)
in vec3 ourColor2;
void main()
{
	FragColor = vec4(ourColor2, 1.0);
};
//Fragment interpolation in the fragment shader