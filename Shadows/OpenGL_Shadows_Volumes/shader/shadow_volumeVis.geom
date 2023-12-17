#version 330
// topology type triangle with adgacencies, 
// if vertex buffer with adjacency information is provided,
// load it correctly in the GS, adjacency is created by it's location 
layout (triangles_adjacency) in;    // six vertices in
layout (triangle_strip, max_vertices = 18) out; // 4 per quad * 3 triangle vertices + 6 for near/far caps

in vec3 PosL[]; // an array of 6 vertices (triangle with adjacency)

uniform vec3 lightPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

float EPSILON = 0.0001;


void main()
{   
    gl_Position = projection * view * model  * vec4(PosL[0], 1.0);
    EmitVertex();
    gl_Position = projection * view * model  * vec4(PosL[2], 1.0);
    EmitVertex();
    gl_Position = projection * view * model  * vec4(PosL[4], 1.0);
    EmitVertex();

    EndPrimitive();
    /*
    

    // render the front cap
    // NOTE: move along the light vector by a small amount
    // avoid bizarre corruption, where the volume hides the front cap
    LightDir = (normalize(PosL[0] - lightPos));
    gl_Position = projection * view * model  * vec4((PosL[0] + LightDir * EPSILON), 1.0);
    EmitVertex();

    LightDir = (normalize(PosL[1] - lightPos));
    gl_Position = projection * view * model  * vec4((PosL[1] + LightDir * EPSILON), 1.0);
    EmitVertex();

    LightDir = (normalize(PosL[2] - lightPos));
    gl_Position = projection * view * model  * vec4((PosL[2] + LightDir * EPSILON), 1.0);
    EmitVertex();
    EndPrimitive();
    */
}