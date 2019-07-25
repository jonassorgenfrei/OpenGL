#version 330 core
//declare type of primitive input
layout (points) in;
/* 
	Can Take any of the following primitive values from a vertex shader.
	-> points: when drawing GL_POINTS primitvies
	-> lines: when drawing GL_LINES or GL_LINE_STRIP
	-> lines_adjacency: GL_LINES_ADJACENCY or GL_LINE_STRIP_ADJACENCY
	-> triangles: GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
	-> triangles_adjacency: GL_TRIANGLE_ADJACENCY or GL_TRIANGLE_STRIP_ADJACENCY
*/

//specify a primitaive type as output
layout (triangle_strip, max_vertices = 5) out;/*
	Can take several primitive values:
	-> points
	-> line_strip
	-> triangle_strip

	-> exprects to set a max. Number of output vertices
	(if exceeded OpenGl won't draw the extra vertices)
*/

/*
	to retrieve data, OpenGl gives a built-in var.: gl_in
	Interface Block!
	in gl_Vertex
	{
		vec4  gl_Position; //Vertex Shader Output
		float gl_PointSize;
		float gl_ClipDistance[];
	} gl_in[];  
	
	Geometry Shader receives all vertices of a primitive as its input
*/

//Interface Block
in VS_OUT {
    vec3 color;
} gs_in[];

//Output for Fragment Shader
out vec3 fColor;

void build_house(vec4 position)
{    
    fColor = gs_in[0].color; // gs_in[0] since there's only one input vertex
    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0); // 1:bottom-left   
    EmitVertex();   									// the vector curr. set to gl_Postion is added to the primitive 
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0); // 2:bottom-right
    EmitVertex();
    gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0); // 3:top-left
    EmitVertex();
    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0); // 4:top-right
    EmitVertex();
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0); // 5:top
	fColor = vec3(1.0, 1.0, 1.0);
    EmitVertex();
	EndPrimitive();							//all emitted vertices for this primitive are combined into the specific output render primitive
											//calling End more often: multi. primitives can be created
}

void main() {    
    build_house(gl_in[0].gl_Position);
}