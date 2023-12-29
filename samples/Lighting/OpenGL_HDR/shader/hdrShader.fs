#version 330 core
/**
 * Reinhard tone mapping 
 * 
 * deviding entire HDR color values to LDR color values
 * evenly balancing them out (spreads out onto LDR)
 *
 * Possible: see the entire range of HDR values
 */

out vec4 FragColor;

in vec2 TexCoords;

uniform bool hdr;
uniform float exposure;
// lower exposure at daylight
// higher exposure at night
// Simliar to the human eye
uniform sampler2D hdrBuffer;

void main() {
	const float gamma = 2.2; // for gamma correction
	vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;

	if(hdr){
		// reinhard tone mapping
		vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
			// gamma correction
		mapped = pow(mapped, vec3(1.0 / gamma));
	
		FragColor = vec4(mapped, 1.0);
	} else {
		// pass-through
		vec3 result = pow(hdrColor, vec3(1.0 / gamma));
		FragColor = vec4(result, 1.0);
	}	
}