#version 450
layout(location = 0) out vec4 OutColor;

layout(location=0) in InterpolantsV {
    vec3 color;
} INV;

layout(location=1) __perprimitiveNV in InterpolantsP {
    vec3 color;
} INP;

void main()
{
    // per vertex color interpolation
    //OutColor = vec4(INV.color, 1.0);
    // pre primitive color (interpolation)
    OutColor = vec4(INP.color, 1.0);
    
    // primitive id
    //OutColor = vec4(1.0);
    //OutColor *= float(gl_PrimitiveID)/4+1.0/4;
}
    