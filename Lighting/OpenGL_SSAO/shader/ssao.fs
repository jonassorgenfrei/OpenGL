#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition; 
uniform sampler2D gNormal; 
uniform sampler2D texNoise;

uniform vec3 samples[64];

// parameters (probably usefull as uniforms to more easily tweak the effect)
uint kernelSize = uint(64);
float radius = 0.5;
float bias = 0.025;
const float PI  = 3.14159265358979;
// tile noise texture over sceen based on screen 
// dimensions divided by noise sizeof
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0); // screen = 1280x720
// tile the noise texture all over the screen, but as the 
// TexCoords vary between 0.0 and 1.0 the texNoise texture 
// won't tile at all. 
// Calculate by how much we'll have to scale the TexCoords coordinates
// by deviding the screen's dimensions by the noise texture size:

/**
 * Invertiert die Binärdarstellung einer Zahl an der Kommastelle
 *
 * @param bits                  Zahl a
 *
 * @return Zahl a mit invertierter Binärdarstellung nach dem Komma
 */
float radicalInverse_VdC(uint bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

uniform mat4 projection;

/**
 * Berechnet auf Basis eines 2D-Vektors einen Zufallswert
 *
 * @param xi                    2D-Vektor als Grundlage des Zufallswertes
 *
 * @return Zufallswert
 */
float rand(vec2 xi){
	return fract(sin(dot(xi.xy, vec2(12.9898,78.233))) * 43758.5453);
}

/**
 * Berechnet einen Vektor in der Hemisphäre um die Y-Achse. Die Verteilung der Vektoren
 * entspricht cos(theta).
 *
 * @param xi                   Zufallsvektor im Intervall [0, 1)
 *
 * @return Vektor in der Hemisphäre um die Y-Achse
 */
vec3 sampleHemisphereCos(float u, float v) { // Y ist oben
        float phi = v * 2.0 * PI;
        float cosTheta = sqrt(1.0 - u);
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

/**
 * Berechnet den i-ten Hammersley-Punkt aus N Punkten
 *
 * @param i                     Zu berechnenden Punkt der Hammersley-Sequenz
 * @param N                     Gesamtanzahl der zu berechnenden Punkte
 *
 * @return i-ter Hammersley-Punkt
 */
vec2 hammersley(uint i, uint N) {
  return vec2(float(i)/float(N), radicalInverse_VdC(i));
}


void main() 
{
	// get input for SSAO algorithm
	vec3 fragPos = texture(gPosition, TexCoords).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
	// random values are repeated all over the screen.

	// create a TBN Matrix to transform any vector 
	// from tangent-space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	// Gramm-Schmidt process
	// creating an orthogonal basis, each time slightly tilted based ont the value 
	// of randomVec 
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	// there is no need to have the TBN matrix exactly aligned to the geometry's 
	// surface (because of the rand. vector), 
	// no need for per-vertex tangent (and bitangent) vectors
		
	// iterate over each of the kernel samples, transform the samples form tangent
	// to view-space, add them to the current fragment position and compare the fragment
	// position's depth with the sample depth stored in the view space-position buffer.
	float occlusion = 0.0;
	for(uint i = uint(0); i < kernelSize; i++)
	{
		vec2 xi = hammersley(i, kernelSize);

        float random = rand(vec2(gl_FragCoord)*xi);

        vec3 sam = sampleHemisphereCos(xi.x, xi.y);

		// get sample position
		vec3 _sample = TBN * sam;//samples[i];	// from tangent to view-space
		_sample = fragPos + _sample * radius;
		// we then add the view-space kernel offset sample to the view-space
		// fragment position
		// multiply the offset sample by radius to increase (or decrease) the
		// effective sample radius of SSAO

		// project sample position (tp sample texture) (to get position on screen/texture)
		// transform sample to screen-space so we can sample the position/depth
		// value of sample as if we were rendering its position directly to the screen
		// as the vector is curretnly in view-space, we'll transform it to 
		// clip-space first using the projection matrix uniform
		vec4 offset = vec4(_sample, 1.0);
		offset = projection * offset;	// from view to clip-space
		offset.xyz /= offset.w;			// perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5;	// transform to range 0.0 - 1.0

		// get sample depth
		// we now can use them to sample the position texture
		float sampleDepth = texture(gPosition, offset.xy).z; // use x + y component to sample the position textue to 
		// retrieve the (depth) z value of the (kernel) sample position as seen from the viewer's perspective

		// Range Check 
		// When Fragment is tested, that is aligned close to the edge of a surface
		// --> we also consider depth values of surfaces far behind the test surfaces
		//     therse values will (incorrectly) contribute to the occlusion factor

		float rangeCheck = smoothstep(0.0, 1.0,radius / abs(fragPos.z - sampleDepth) );
		// smoothstep function, that smoothly interpolates its thid parameter between the first
		// and second paramter's range, returning 0.0 if less or equal to its first paramter and 1.0
		// if equal or higher to its second parameter --> avoid unattractive borders

		// check if the sample's current depth value is larger than the stored
		// depth value and if so, we add to the final contribution factor
		// makes sure fragment contributes to the occlusionfactor if its depth values is within the samples radius
		occlusion += (sampleDepth >= _sample.z + bias ? 1.0 : 0.0) * rangeCheck; // add small bias to the original fragment's depth value 
		// solves acne effects (occure on complex scenes)

	}
	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = occlusion;
}