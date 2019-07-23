#version 330 core 
in vec4 FragPos; // input from the 

uniform vec3 lightPos;
uniform float far_plane;

/**
 * Calculate own (linear) depth as the linear distance between each fragment position and the lightPos
 * source's position
 *		--> later shadow calculations will be a little bit more intuitive
 */
void main() 
{
	// for billboards
	//if(alpha < threshold)
	// discard;
	// get distance between ragment and light source
	float lightDistance = length(FragPos.xyz - lightPos);
	// map to [0;1] range by dividing by far_plane
	lightDistance = lightDistance / far_plane;

	// write this as modified depth
	gl_FragDepth = lightDistance;
}