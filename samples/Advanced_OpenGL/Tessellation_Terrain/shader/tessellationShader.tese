/*
 * Tessellation Evaluation Shader TES
 *	erhaelt die Position der neuen Vertices im Abstract Patch auf der vordef. Variable gl_TessCoord
 *	& Attribute des Output Patches als in Variable.
 *		Der TES kann die Attribute der Vertices abhaengig von der Position des Vertex im Abstract Patch selbst interpolieren.
 *
 *		Berechnet 3D Koordinate des Punkts
 *
 *	Eigenschaft:
 *		pro neuerzeugten Vertex aufgerufen
 *		nest. Typ des Abstract Patch, den Spacing mode und die Vertex Reihenfolge (CW oder CCW)
 *		Zugriff auf alle Verties des Output patches
 *		interpol. Attribute von TCS + leitet sie weiter
 */

 #version 430 core

 // definiert die Größe des Abstract Patches (Hier: Traingles=3Vertices)
 // 1. param - Unterteilung in Triangles, Isolines oder Quads (Domain)
 // 2. param - Immer gleiche Abstände der erzeugten Primitive
 //				alternativ: 
 //				equal_spacing <- creates subd of equal size
 //				fractional_even_spacing <- creates odd num of subd broken into long&short segments   (smoother transition between the lengths)
 //				fractional_odd_spacing <- creates even num of subd broken into long&short segments   (smoother transition between the lengths)
 // 3. param - Primitive im oder gegen der Uhrzeigersinn
 // 
 layout(quads, fractional_odd_spacing, ccw) in;


 // Attribute des Output Patches (Länge abhängig vom Output Patch)
 in TCS_OUT {
	vec2 texCoords;
 } tes_in[];

 out TES_OUT {
	float Height;
	vec3 normal;
 } tes_out;

 uniform sampler2D heightMap;

 uniform mat4 model;
 uniform mat4 view;
 uniform mat4 projection;
 uniform vec2 texelSize;

 void main() {
	/*
	 * Bei Quads:
	 * gl_TessCoord stellt eine 2D Koordinante zur bilinearen Interpolation bereit
	 */

	/*
	 * Bei Lines:
	 * gl_TessCoord stellt eine 2D Koordinante bereit (X definiert die Position auf der Linie und Y left die Linie Fest)
	 */
	float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
	//float width = 2624;
	//float height = 1756;
    vec2 t00 = tes_in[0].texCoords;
    vec2 t01 = tes_in[1].texCoords;
    vec2 t10 = tes_in[2].texCoords;
    vec2 t11 = tes_in[3].texCoords;

	// bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

	// sample texture value for height
    tes_out.Height = texture(heightMap, texCoord).x * 64.0 - 16.0;

	// retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

	// compute patch surface normal
    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 normal = normalize( vec4(cross(uVec.xyz, vVec.xyz), 0) );

	// bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10; 
    vec4 p = (p1 - p0) * v + p0;
	
	// displace point along normal
	p += normal * tes_out.Height;

	float height_scale = 64;
	float left  = texture(heightMap, texCoord + vec2(-texelSize.x, 0.0)).r * height_scale * 2.0 - 1.0;
	float right = texture(heightMap, texCoord + vec2( texelSize.x, 0.0)).r * height_scale * 2.0 - 1.0;
	float up    = texture(heightMap, texCoord + vec2(0.0,  texelSize.y)).r * height_scale * 2.0 - 1.0;
	float down  = texture(heightMap, texCoord + vec2(0.0, -texelSize.y)).r * height_scale * 2.0 - 1.0;

	// TODO: FIX NORMAL!
	tes_out.normal = normalize(vec3(left - right, 2.0, up - down));
	
	// output patch point position in clip space
    gl_Position = projection * view * model * p;
 }

	 

