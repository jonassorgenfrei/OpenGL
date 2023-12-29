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

// Emit a quad using a triangle strip
void EmitQuad(vec3 StartVertex, vec3 EndVertex)
{    
    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    vec3 LightDir = normalize(StartVertex - lightPos);   
    gl_Position = projection * view * model  * vec4((StartVertex + LightDir * EPSILON), 1.0);
    EmitVertex();
 
    // Vertex #2: the starting vertex projected to infinity
    gl_Position = projection * view * model  * vec4(LightDir, 0.0);
    EmitVertex();
    
    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    LightDir = normalize(EndVertex - lightPos);
    gl_Position = projection * view * model  * vec4((EndVertex + LightDir * EPSILON), 1.0);
    EmitVertex();
    
    // Vertex #4: the ending vertex projected to infinity
    gl_Position = projection * view * model  * vec4(LightDir , 0.0);
    EmitVertex();

    EndPrimitive();            
}


void main()
{
    vec3 e1 = PosL[2] - PosL[0];
    vec3 e2 = PosL[4] - PosL[0];
    vec3 e3 = PosL[1] - PosL[0];
    vec3 e4 = PosL[3] - PosL[2];
    vec3 e5 = PosL[4] - PosL[2];
    vec3 e6 = PosL[5] - PosL[0];

    // calulcate normal of the current triangle
    vec3 Normal = normalize(cross(e1,e2));
    vec3 LightDir = normalize(lightPos - PosL[0]);


    // Handle only light facing triangles
    if (dot(Normal, LightDir) > 0.00001) {    // check if triangle faces the light use a small epsilon due to the floating point error
        
        // for every adjacent triangle do the test
        Normal = cross(e3,e1);

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[0];
            vec3 EndVertex = PosL[2];
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e4,e5);
        LightDir = lightPos - PosL[2];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[2];
            vec3 EndVertex = PosL[4];
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e2,e6);
        LightDir = lightPos - PosL[4];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[4];
            vec3 EndVertex = PosL[0];
            EmitQuad(StartVertex, EndVertex);
        }

        // render the front cap
        // NOTE: move along the light vector by a small amount
        // avoid bizarre corruption, where the volume hides the front cap
        LightDir = (normalize(PosL[0] - lightPos));
        gl_Position = projection * view * model  * vec4((PosL[0] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = (normalize(PosL[2] - lightPos));
        gl_Position = projection * view * model  * vec4((PosL[2] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = (normalize(PosL[4] - lightPos));
        gl_Position = projection * view * model  * vec4((PosL[4] + LightDir * EPSILON), 1.0);
        EmitVertex();
        EndPrimitive();
 
        // render the back cap
        // project original vertices into infinity along the light direction
        // emit verteices in reversed order
        LightDir = PosL[0] - lightPos;
        gl_Position = projection * view * model  * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosL[4] - lightPos;
        gl_Position = projection * view * model  * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosL[2] - lightPos;
        gl_Position = projection * view * model  * vec4(LightDir, 0.0);
        EmitVertex();

        EndPrimitive();
    }
}