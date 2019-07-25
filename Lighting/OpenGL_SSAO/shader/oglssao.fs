#version 330 

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D gPositionMap;
uniform float gSampleRad;
uniform mat4 projection;

const int MAX_KERNEL_SIZE = 64;
uniform vec3 gKernel[MAX_KERNEL_SIZE]; // array of uniform vectors

void main() 
{
	vec3 Pos = texture(gPositionMap, TexCoord).xyz; // fetch Viewspace Position

	float AO = 0.0;

	for(int i = 0; i < MAX_KERNEL_SIZE; i++)
	{
		vec3 samplePos = Pos + gKernel[i]; // generate a random points
		vec4 offset = vec4(samplePos, 1.0);; // make it a 4-vector
		offset = projection * offset; // project on the near clipping plane
		offset.xy /= offset.w;	// perform perspective devide 
		offset.xy = offset.xy * 0.5 + vec2(0.5); // transform to (0,1) range

		float sampleDepth = texture(gPositionMap, offset.xy).b;

		if (abs(Pos.z - sampleDepth) < gSampleRad) {// check that the distance is not too far ogg
            AO += step(sampleDepth,samplePos.z);// compare depth of the virtual point with the one from the actual geometry
        }

	}
	AO = AO/float(MAX_KERNEL_SIZE);
	// reverse AO to multiply with color of lighted pixels
	//FragColor = vec4(AO);

	FragColor = vec4(pow(AO, 2.0)); //artist Variable
}