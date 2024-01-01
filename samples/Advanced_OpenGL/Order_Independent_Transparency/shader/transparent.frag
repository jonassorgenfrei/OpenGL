#version 430 core

// shader collects pixel infromation for the composite pass

// shader outputs

// first render target which is used to accumulate pre-multiplied color values
layout (location = 0) out vec4 accum; // must have at least RGBA16F precision
// second render target which is used to store pixel revealage
layout (location = 1) out float reveal; // must have at least R8 precision

// material color
uniform vec4 color;

void main()
{
	// NOTE: this doesn't inlcude any lighting caluclations
	// they should be implmented here and the results should be used
	// instead of the color

	// weight function
	// the color-based factor
	// avoids color pollution from the edges of wispy clouds. the z-based
	// factor gives precedence to nearer surfaces
	// needs adjusted weighting function for higher depth/alpha complexity 
	float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
	
	// store pixel color accumulation
	accum = vec4(color.rgb * color.a, color.a) * weight; //  pre-multiplied alpha and weight
	
	// store pixel revealage threshold
	reveal = color.a;
}