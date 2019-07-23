#version 420 core 

out vec4 FragColor;

//layout (depth_greater) out float gl_FragDepth; // only from OpenGL 4.2 or higher

/*
	any 		The default value. Early depth testing is disabled and you lose most performance.
	greater 	You can only make the depth value larger compared to gl_FragCoord.z.
	less 		You can only make the depth value smaller compared to gl_FragCoord.z.
	unchanged 	If you write to gl_FragDepth, you will write exactly gl_FragCoord.z.

	on greater or less, openGL can still do an early depth test 
*/

/* input interface block */
in VS_OUT //block name => has to be equal to the one in the fragmentshader
{
    float color;
	vec2 TexCoords;
} fs_in; //instance  name => can be anything 

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

void main() {
	
	//FragColor = vec4(color, 0.0, 0.0, 1.0);
	
	//gl_FragCoord
	// z componnt is equal to the depth value of that particular fragment

	//calculate different color based on window coordinate of the fragment
	if(gl_FragCoord.x < 640)
		FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	else
		FragColor = vec4(0.0, 1.0, 0.0, 1.0);

	//gl_FrontFacing
	// just possible when not using face culling
	if(gl_FrontFacing){
		//if fragment is part of a front face
        FragColor = texture(frontTexture, fs_in.TexCoords);
    } else {
		//if fragment is part of a back face
        FragColor = texture(backTexture, fs_in.TexCoords);
	}

	//gl_FragDepth
	//set depth value of the fragment; otherwise variable will automatically take its value from gl_FragCoord.z
	if(gl_FragCoord.x < 640)
		gl_FragDepth = 0.0;
	else 
		gl_FragDepth = gl_FragCoord.z;
	//disadvantage: OpenGL disables all early depth testing as soon as we write to gl_FragDepth
	// this is because OpenGL cannot know what depth value the fragment will have before we run the fragment shader, since the fragment shader might
	// completely change this depth value

	//gl_FragDepth = gl_FragCoord.z + 0.1;

}