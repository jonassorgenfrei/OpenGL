#version 330 core

/**
 * Gaussian blur
 *	weighted values, smaller weights the larger the distance to the fragment.
 *  Becomes quickly extremely heavy on performance. Samples 1024 times for each fragment
 *
 *		Two-pass Gaussian blur
 *			seperate the 2d equation into 2 smaller equations
 *				- one describes the horizontal weights 1.
 *				- one describes the vertical weights 2.
 *			saves incredible amount of performance only 32+32 samples (comp. 1024 samples)
 */

 out vec4 FragColor;

 in vec2 TexCoords;

 uniform sampler2D image;

 uniform bool horizontal; // split boolean
 uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

 void main() {
	vec2 tex_offset = 1.0 / textureSize(image, 0); // get size of single texel
	vec3 result = texture(image, TexCoords).rgb * weight[0];
	if(horizontal) // split gaussian filter into a horizontal and .. 
	{
		for(int i = 1; i < 5; ++i)
		{
			result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	} else { // ... a vertical section
		for(int i = 1; i < 5; ++i)
		{
			result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}
	}
	FragColor = vec4(result, 1.0);
 }
