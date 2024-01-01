#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 TexCoordOut;
// tells OpenGL to which colorbuffer of the currently active FB to render

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

/**
 * it's extremly important to keep all variables in the same coord. space!!!!!!!
 */
void main() 
{
	// store the frag position vec. in the first gbuffers texture
	gPosition = FragPos;
	// store per-fragment normals into the gBuffer
	gNormal = normalize(Normal);
	// diffuse per-fragment color 
	gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
	// specular intensity in gAlbedoSpec's alpha component
	gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;

	TexCoordOut = vec3(TexCoords, 0.0); 
}