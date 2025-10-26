#version 330   
// ----------------------------------------------------------------------------
//
// Attributes
//
// ----------------------------------------------------------------------------

in vec3 fTexCoord; 

out vec4 FragColor;            


uniform float alphaThreshold;   

uniform sampler2D tex;         

uniform sampler2DArray texArray;

uniform bool DrawTexCoords;

// ----------------------------------------------------------------------------
//
// Funktionen
//
// ----------------------------------------------------------------------------

/**
 * Einsprungpunkt für den Fragment-Shader
 */
void main() {
    vec3 invTexCoord = vec3(fTexCoord.x, 1 - fTexCoord.y, fTexCoord.z);
    FragColor = texture(texArray, invTexCoord, 0);
    if(DrawTexCoords){
        FragColor = vec4(invTexCoord, 1);
    }

    if(FragColor.a < alphaThreshold)
        discard;
}
