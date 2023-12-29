#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

/* Postprocessing Filters */
const float offset = 1.0 / 300;

//inverse colors
vec4 inversion(vec3 color){
	return vec4(vec3(1.0 - color), 1.0);
}

//remove all colors from the scene except the white, grey & black colors
// weighted => gewichtung
vec4 greyscale(vec3 color, int weighted) {
	float average;
	if(weighted != 0){
		//the human eye tends to be more sensitive to green colors and the least to blue 
		//=> most physically accurate results => weighted channels
		average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
	} else {
		//unweighted
		average = (color.r + color.g + color.b) / 3.0;
	}
	return vec4(average, average, average, 1.0);
}

vec4 kernelM(float[9] kernel){
	vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), vec2( 0.0f,    offset),	vec2( offset,  offset), 
       // top-left				top-center				top-right
        vec2(-offset,  0.0f),   vec2( 0.0f,    0.0f),	vec2( offset,  0.0f),
        // center-left			center-center			center-right      
        vec2(-offset, -offset), vec2( 0.0f,   -offset),	vec2( offset, -offset)
		// bottom-left			bottom-center			bottom-right
    );
	//each surrounding texture coordinate, constante value, likely customized 

	vec3 sampleTex[9];
	for(int i = 0; i<9; i++){
		sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
	}
	vec3 col = vec3(0.0);
	for(int i = 0; i<9; i++)
		col += sampleTex[i] * kernel[i];

	return vec4(col, 1.0);
}

void main()
{
	//sharpens each color value by sampling all surrounding pixels in an interesting way
	float sharpenKernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

	float sharpenKernel2[9] = float[](
		 0, -1,  0,
		-1,  5, -1,
		 0, -1,  0
	);
	
	/* Gaussian blur 3x3 (approximation) */
	float gaussianBlurKernel[9] = float[](
		1.0 / 16, 2.0 / 16, 1.0 / 16,
		2.0 / 16, 4.0 / 16, 2.0 / 16,
		1.0 / 16, 2.0 / 16, 1.0 / 16  
	);

	/* Box blur (normalized) */
	float boxBlurKernel[9] = float[](
		1.0/9, 1.0/9, 1.0/9,
		1.0/9, 1.0/9, 1.0/9,
		1.0/9, 1.0/9, 1.0/9
	);

	/* Edgedetection */
	float edgeDetKernel1[9] = float[](
		1,  1,  1,
        1, -8,  1,
        1,  1,  1
	);

	float edgeDetKernel2[9] = float[](
		1,  0, -1,
        0,  0,  0,
       -1,  0,  1
	);

	float edgeDetKernel3[9] = float[](
		0,  1,  0,
        1, -4,  1,
        0,  1,  0
	);

	vec3 col = texture(screenTexture, TexCoords).rgb;
	vec4 res = vec4(col, 1.0);
	//res = kernelM(gaussianBlurKernel);
	//res = inversion(res.rgb);
	//res = greyscale(res.rgb, 1);
    FragColor = res;
}