#version 330   

layout (triangles) in;
layout (triangle_strip, max_vertices = 64) out;

// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec4 gPosition[];               
in vec3 gNormal[];                 

out vec3 fTexCoord;                 

// ----------------------------------------------------------------------------
//
// Uniforms
//
// ----------------------------------------------------------------------------

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float NormalLength;        
uniform int Subdivs;                
uniform int Billboards;             
uniform float Time;                 
uniform bool Animation;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

/**
 * Berechnet auf Basis eines 2D-Vektors einen Zufallswert
 *
 * @param xi                    2D-Vektor als Grundlage des Zufallswertes
 *
 * @return Zufallswert
 */
float rand(vec2 xi){
    return abs(fract(sin(dot(xi.xy, vec2(12.9898,78.233))) * 43758.5453));
}

vec3 randomBarycentric(vec2 xi) {
    float r1 = rand(xi + vec2(0));
    float r2 = rand(xi + vec2(322, 848));
    float r3 = rand(xi + vec2(21, 987));
    return vec3(r1, r2, r3) / (r1 + r2 + r3);
}

/**
 * Einsprungpunkt für den Geometry Shader
 */
void main() {
    float nLength = NormalLength / float(Subdivs);
    // Schleife über alle eingehenden Vertices
    for (int i = 0; i < Billboards; i++) {
        vec3 b = randomBarycentric(gPosition[0].xz * (i + 1));
        vec4 p0 = gPosition[0] * b[0] + gPosition[1] * b[1] + gPosition[2] * b[2];
        
        float bend = 0;
        if(Animation)
            bend = sin((p0.x + p0.z) + b[0] * 324.48 + Time) * (0.25);        

        vec3 normal = gNormal[0] * b[0] + gNormal[1] * b[1] + gNormal[2] * b[2];
        vec3 offset = normalize(cross(normal, vec3(rand(p0.xz * (i + 1)) * 2 - 1, 
                        0, rand(p0.xz + vec2(546, 35.54) * (i + 1)) * 2 - 1))) * NormalLength / 4.;
        vec4 p1 = p0 + vec4(offset, 0);
        p0 = p0 + vec4(-offset, 0);
        
        float arrayTexture = rand(p0.xz);
        for(int i = 0; i < Subdivs + 1; i++){
            gl_Position = projection*view*model*p0;
            fTexCoord = vec3(0, float(i) / float(Subdivs), arrayTexture * 3);
            EmitVertex();

            gl_Position = projection*view*model*p1;
            fTexCoord = vec3(1, float(i) / float(Subdivs), arrayTexture * 3);
            EmitVertex();
            
            normal += normalize(cross(normal, p0.xyz - p1.xyz)) * bend;

            p0 += vec4(normal * nLength, 0);
            p1 += vec4(normal * nLength, 0);

        }
        // Abschließen des Primitivs. Die nächsten Aufrufe von EmitVertex erzeugen einen neuen line strip.
        EndPrimitive();
    }
}
