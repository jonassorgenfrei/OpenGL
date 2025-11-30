#version 330
// topology type triangle with adgacencies, 
// if vertex buffer with adjacency information is provided,
// load it correctly in the GS, adjacency is created by it's location 
layout (triangles_adjacency) in;    // six vertices in
layout (triangle_strip, max_vertices = 18) out; // 4 per quad * 3 triangle vertices + 6 for near/far caps

in vec3 PosL[]; // an array of 6 vertices (triangle with adjacency)

uniform vec3 gLightPos;
uniform mat4 gWVP;

// match the primary shadow volume shader to avoid visualization offsets
const float EPSILON = 0.0001;

void EmitQuad(vec3 StartVertex, vec3 EndVertex)
{
    vec3 startDir = normalize(StartVertex - gLightPos);
    vec3 endDir   = normalize(EndVertex   - gLightPos);

    gl_Position = gWVP * vec4((StartVertex + startDir * EPSILON), 1.0);
    EmitVertex();

    gl_Position = gWVP * vec4(startDir, 0.0);
    EmitVertex();

    gl_Position = gWVP * vec4((EndVertex + endDir * EPSILON), 1.0);
    EmitVertex();

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

    vec3 Normal = normalize(cross(e1,e2));
    vec3 LightDir = normalize(gLightPos - PosL[0]);

    if (dot(Normal, LightDir) > 0) {
        Normal = cross(e3,e1);

        if (dot(Normal, LightDir) <= 0) {
            EmitQuad(PosL[0], PosL[2]);
        }

        Normal = cross(e4,e5);
        LightDir = gLightPos - PosL[2];

        if (dot(Normal, LightDir) <= 0) {
            EmitQuad(PosL[2], PosL[4]);
        }

        Normal = cross(e2,e6);
        LightDir = gLightPos - PosL[4];

        if (dot(Normal, LightDir) <= 0) {
            EmitQuad(PosL[4], PosL[0]);
        }

        // front cap
        LightDir = normalize(PosL[0] - gLightPos);
        gl_Position = gWVP * vec4((PosL[0] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = normalize(PosL[2] - gLightPos);
        gl_Position = gWVP * vec4((PosL[2] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = normalize(PosL[4] - gLightPos);
        gl_Position = gWVP * vec4((PosL[4] + LightDir * EPSILON), 1.0);
        EmitVertex();
        EndPrimitive();
 
        // back cap (extruded)
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
