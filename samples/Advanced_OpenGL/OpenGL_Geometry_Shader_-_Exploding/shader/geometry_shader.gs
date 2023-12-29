#version 330 core
//declare type of primitive input
layout (triangles) in;
/* 
	Can Take any of the following primitive values from a vertex shader.
	-> points: when drawing GL_POINTS primitvies
	-> lines: when drawing GL_LINES or GL_LINE_STRIP
	-> lines_adjacency: GL_LINES_ADJACENCY or GL_LINE_STRIP_ADJACENCY
	-> triangles: GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
	-> triangles_adjacency: GL_TRIANGLE_ADJACENCY or GL_TRIANGLE_STRIP_ADJACENCY
*/

//specify a primitaive type as output
layout (triangle_strip, max_vertices = 3) out;
/*
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
    vec2 texCoords;
} gs_in[];

//Output for Fragment Shader appropriate texture coord.
out vec2 TexCoords;

uniform float time;
uniform int explodeB;

vec4 explode(vec4 position, vec3 normal)
{
	float magnitude = 2.0;
	vec3 direction = vec3(0.0);
	if(explodeB == 1){
		//direction = normal * ((sin(time) + 1.0)/2.0) * magnitude;
		direction = normal * time * magnitude;	
	}

	direction.y *= 9.81;
	if(direction.y > 0)
		direction.y *= -1;
	return position + vec4(direction, 0.0);
}

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}  

void main() {   
	vec3 normal = GetNormal();

	gl_Position = explode(gl_in[0].gl_Position, normal);
	TexCoords = gs_in[0].texCoords;
	EmitVertex();
	gl_Position = explode(gl_in[1].gl_Position, normal);
	TexCoords = gs_in[1].texCoords;
	EmitVertex();
	gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].texCoords;
    EmitVertex();
    EndPrimitive();
}