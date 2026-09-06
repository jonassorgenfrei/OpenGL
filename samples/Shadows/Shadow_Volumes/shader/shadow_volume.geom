#version 330
// Use triangle-adjacency input topology.
// The index buffer supplies one opposite vertex per triangle edge,
// allowing the geometry shader to identify silhouette edges.
layout (triangles_adjacency) in;    // six vertices in
layout (triangle_strip, max_vertices = 18) out; // 4 per quad * 3 triangle vertices + 6 for near/far caps

in vec3 PosL[]; // an array of 6 vertices (triangle with adjacency)

uniform vec3 gLightPos;
uniform mat4 gWVP;

// Emit a quad using a triangle strip
void EmitQuad(vec3 StartVertex, vec3 EndVertex)
{
    vec3 startDir = normalize(StartVertex - gLightPos);
    vec3 endDir   = normalize(EndVertex   - gLightPos);

    // Vertex #1: the starting vertex on the occluder surface. The application
    // applies a depth-buffer-space polygon offset during the stencil pass.
    gl_Position = gWVP * vec4(StartVertex, 1.0);
    EmitVertex();

    // Vertex #2: the starting vertex extruded away from the light
    gl_Position = gWVP * vec4(startDir, 0.0);
    EmitVertex();

    // Vertex #3: the ending vertex on the occluder surface
    gl_Position = gWVP * vec4(EndVertex, 1.0);
    EmitVertex();

    // Vertex #4: the ending vertex extruded away from the light
    gl_Position = gWVP * vec4(endDir, 0.0);
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

    // Calculate the current triangle normal.
    vec3 Normal = normalize(cross(e1,e2));
    vec3 LightDir = normalize(gLightPos - PosL[0]);


    // Only light-facing triangles contribute a front cap and silhouette edges.
    if (dot(Normal, LightDir) > 0) {
        
        // for every adjacent triangle do the test
        Normal = cross(e3,e1);

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[0];
            vec3 EndVertex = PosL[2];
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e4,e5);
        LightDir = gLightPos - PosL[2];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[2];
            vec3 EndVertex = PosL[4];
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e2,e6);
        LightDir = gLightPos - PosL[4];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosL[4];
            vec3 EndVertex = PosL[0];
            EmitQuad(StartVertex, EndVertex);
        }

        // Render the front cap at the exact occluder position. A fixed
        // object-space epsilon becomes unreliable as camera distance changes;
        // the stencil pass supplies a depth-buffer-space polygon offset instead.
        gl_Position = gWVP * vec4(PosL[0], 1.0);
        EmitVertex();

        gl_Position = gWVP * vec4(PosL[2], 1.0);
        EmitVertex();

        gl_Position = gWVP * vec4(PosL[4], 1.0);
        EmitVertex();
        EndPrimitive();
 
        // render the back cap
        // project original vertices to infinity along the light direction (reversed order)
        LightDir = PosL[0] - gLightPos;
        gl_Position = gWVP * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosL[4] - gLightPos;
        gl_Position = gWVP * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosL[2] - gLightPos;
        gl_Position = gWVP * vec4(LightDir, 0.0);
        EmitVertex();

        EndPrimitive();
    }
}
