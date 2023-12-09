#version 330 core
/**
 * Responsible for tramsforming all world-space vertices to 6 different light spaces.
 */
 layout (triangles) in;	// input 1 triangle (3 triangle vertices)
 layout (triangle_strip, max_vertices=18) out; // output 6 triangles (6*3 vertices)

 uniform mat4 shadowMatrices[6]; // uniform array of light space transformations matrices.

 out vec4 FragPos; // FragPos from GS (output per emitvertex)

 /**
  * transforming vertices to the light spaces
  */
 void main() {
	for(int face = 0; face < 6; ++face)  // iterate over 6 cubemap face
	{
		gl_Layer = face; // built-in variable that specifies to which face of the cubemap to render in
		for(int i = 0; i < 3; ++i) // for each triangle's vertices
		{
			FragPos = gl_in[i].gl_Position;
			gl_Position = shadowMatrices[face] * FragPos;
			EmitVertex();
		}
		EndPrimitive();
	}
 }