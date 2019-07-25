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
 // 1. param - Unterteilung in Dreiecke, Isolines oder Quads (Domain)
 // 2. param - Immer gleiche Abstände der erzeugten Primitive
 //				alternativ: fractional_even_spacing fractional_odd_spacing   <= smoother transition between the lengths
 // 3. param - Primitive im oder gegen der Uhrzeigersinn
 // 
 layout(triangles, equal_spacing, ccw)in;


 // Attribute des Output Patches (Länge abhängig vom Output Patch)
 in TCS_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
 } tes_in[];

 out TES_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
 } tes_out;
  
 uniform sampler2D depthMap;
 uniform float gDispFactor;

 uniform mat4 projection;
 uniform mat4 view;

 // Interpolations Functionen
 vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
 }
 
 vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
} 

 // Bei der Auswertung steht auf gl_TessCoord die Baryzentrische Koordinate des neuen Vertex bereit
 vec4 interpolate4D(vec4 v0, vec4 v1, vec4 v2){
	return	gl_TessCoord.x*v0		// Baryzentrische Interpolation der Attribute		Baryzentrische Koordinate in gl_TessCoord.xy
		+	gl_TessCoord.y*v1		// des Output Patches auf Basis der Position
		+	gl_TessCoord.z*v2;		// des neuen Vertex im Abstract Patch.
 }
 
 void main() {
	// Interpolate the attributes of the output vertex using the barycentric coordinates
	tes_out.texCoords = interpolate2D(tes_in[0].texCoords, tes_in[1].texCoords, tes_in[2].texCoords);
	tes_out.normal = interpolate3D(tes_in[0].normal, tes_in[1].normal, tes_in[2].normal);
	tes_out.normal = normalize(tes_out.normal);
	tes_out.positionW = interpolate3D(tes_in[0].positionW, tes_in[1].positionW, tes_in[2].positionW);

	// Displace the vertex along the normal
	float Displacement = 1-texture(depthMap, tes_out.texCoords.xy).x;
	tes_out.positionW += tes_out.normal * Displacement * gDispFactor;

	gl_Position = projection * view * vec4(tes_out.positionW,1.0);
 }

 /*
	//aus altern. Quelle
	void main() {
		float u = gl_TessCoord.x; // Geht bayrizentrische Tessellierungs-Koordinaten (u,V)
		float v = gl_TessCoord.y;

		vec4 tmp1 = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u); // Nutze 3D koord. des PAtces um neue 3D Position des Punktes zu bestimmen
		vec4 tmp2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u); // (Hier: Patch ist ein einfaches Rechteck)

		vec4 position = mix(tmp1, tmp2, v);
		// modify position (extrude along normal ... )
		gl_Position = projection * view * model * position;	// gibt modifizierte Position aus; (Berechnung in Weltkoord.)
	}
 */


	/*
	 * Bei Quads:
	 * gl_TessCoord stellt eine 2D Koordinante zur bilinearen Interpolation bereit
	 */

	/*
	 * Bei Lines:
	 * gl_TessCoord stellt eine 2D Koordinante bereit (X definiert die Position auf der Linie und Y left die Linie Fest)
	 */
	 

