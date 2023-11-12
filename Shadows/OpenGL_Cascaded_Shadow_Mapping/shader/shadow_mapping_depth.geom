#version 430 core

layout(triangles, invocations = 5) in;	// instance shader, specify layer of the array texture & shadow matrix
layout(triangle_strip, max_vertices = 3) out;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};

//uniform mat4 lightSpaceMatrices[16];

void main()
{
	// create multiple layers of geometry at the same time
	// to layered rendering
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}  