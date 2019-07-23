#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

/* Uniform buffer objects */
//global uniform variables 
//remain the same over several shader programs
layout (std140) uniform Matrices //std140 => current defined uniform block uses a specific memory layout for its content; it's sets the "uniform block layout"
{						//	base alignment		// aligned offset
	mat4 projection;	//	16					// 0
	mat4 view;			//	16					// 16
};

/* Uniform Block Layout */
//------------------------
// stored in a buffer object => reserved piece of memory
// we need to tell OpenGL what parts of the memory corresponds to which variables in the shader
// knowing: size (bytes) & offset (from the start of the block): place them in the buffer in their respective order

//by default: opengl uses a uniform memory layout called a shared layout (shared -> once the offsets 
// are defined by the hardware, they are consistently shared between multiple programs)
// => GLSL is allowed to reposition the uniform variables for optimization as long as the variables' order remains intact
// more complicated e.g. glGetUniformIndices .> query information about offset of each uniform variable
// => some space-saving optimizations !!!


//std140 => explicitly states the memory layout for each variable type by stating their respective offsets 
// governed by a set of rules
// since this is explictly mentioned we can manually figure out the offsets for each variable
// each variable has a base alignment => equal to the space a variable takes (including padding) within a uniform block
//	=> calculated using the std140 layout rules
//		for each variable; calculate its aligned offset which is the byte offset of a variable from the start of the block
//		=> aligned byte offset of a variable must be equal to a multiple of its base alignment
//	not the most efficient => std140 layout => guarantee us that the memory layout remains the same over each prog
//
// most common rules:>
//	each vairable type in GLSL (int, float, bool) )> defined to be four-byte quantitites with each entitiy of 4
//	bytes being represented as N
//
//TYPE							LAYOUT RULE
//--------------------------------------------
//Scalar e.g. int or bool		each scalar has a base alignment of N.
//Vector						either 2N or 4N. This means that a vec3 has a base alignment of 4N.
//Array of scalars or vectors	each element has a base alignment equal to that of a vec4.
//Matrices						stored as a large array of column vectors, where each of those vectors has a
//								base alignment of vec4.
//Struct						equal to the computed size of its elements according to the previous rules, but padded to a multiple of the size of a vec4

//layout (std140) uniform ExampleBlock
//{
//                     // base alignment  // aligned offset
//    float value;     // 4               // 0 
//    vec3 vector;     // 16              // 16  (must be multiple of 16 so 4->16)
//    mat4 matrix;     // 16              // 32  (column 0)
//                     // 16              // 48  (column 1)
//                     // 16              // 64  (column 2)
//                     // 16              // 80  (column 3)
//    float values[3]; // 16              // 96  (values[0])
//                     // 16              // 112 (values[1])
//                     // 16              // 128 (values[2])
//    bool boolean;    // 4               // 144
//    int integer;     // 4               // 148
//}; 
// The aligned byte offset of a variable must be equal to a multiple of its base alignment.

// other layout: packed -> no guarantee that the layout remains the same between programs 
// (not shared) because it allows the compiler to optimize variables away from the uniform block 
// which might differ per shader

//manually set the uniform that are unique per shader
uniform mat4 model;

//Interface blocks
/* helping organize these variables */
out VS_OUT //interface called vs_out
{
	float color;
    vec2 TexCoords;
} vs_out;


void main()
{
	//outputvariables since, their value is read as output from the vertex shader
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	gl_PointSize = gl_Position.z;
	// float variable to set the point's width and height in pixels
	// e.g. particle generation
	
	//inputvariable gl_VertexID; holds current ID of the vertex we're drawing
	//indexed rendering (glDrawElements) => variable holds current index of vertex
	//without indices (glDrawArrays) => variable holds number of current processed vertex since the start (of render call)
	vs_out.color = gl_VertexID*0.03;

	vs_out.TexCoords = aTexCoords;
}