#version 430 core

out vec4 FragColor;


in VS_OUT {
	float Height;
	vec3 Position;
} fs_in;

 uniform sampler2D diffuseMap;


void main() 
{
    float h = (fs_in.Height + 16)/32.0f;	// shift and scale the height into a grayscale value
    FragColor = vec4(h, h, h, 1.0);
}