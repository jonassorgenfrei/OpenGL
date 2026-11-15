#version 450
layout(location = 0) out vec4 OutColor;

layout(location=0) in InterpolantsV {
    vec3 color;
} INV;

layout(location=1) __perprimitiveNV in InterpolantsP {
    vec3 color;
} INP;

uniform int outputMode;

void main()
{
    if (outputMode == 0) {
        // Per-vertex color interpolation.
        OutColor = vec4(INV.color, 1.0);
    }
    else if (outputMode == 1) {
        // Per-primitive color.
        OutColor = vec4(INP.color, 1.0);
    }
    else {
        // Primitive ID as a grayscale value.
        OutColor = vec4(1.0);
        OutColor *= float(gl_PrimitiveID) / 4.0 + 1.0 / 4.0;
    }
}
    
