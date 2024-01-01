#version 330 core

layout(points) in; // input is points
layout(triangle_strip, max_vertices = 6) out; // output is triangle (3 vertices)

in float[] geoValue;
out float outValue;

void main()
{
    for (int i = 0; i < 6; i++) {
        outValue = geoValue[0] + i;
        EmitVertex(); // for each incomming point a triangle is generated (2 more points)
    }

    EndPrimitive();
}