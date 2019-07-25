/*
 * Tessellation Control Shader - TCS
 *	optional
 *	ermoeglicht eine genauere Steuerung des Tessellation Prozesses
 *
 *	Aufgabe:
 *		Aus Input Patch -> Output Patch fuer den TES zu erzeugen & Staerke bestimmen
 *
 *	Pro Vertex des Output Patches aufgerufen
 *		Zugriff auf alle Vertices des input Patches
 *		Legt Anzahl der Vertices pro OutputPatch fest
 *		Best. Unterteilungshaeufigkeit des Abstract patch
 *		Leitet Attribute an den TES weiter 
 *
 */
 #version 430 core

 // Definiert die Anzahl von Kontrollpunkten des Output Patches
 layout (vertices = 3) out;

 // Attribute des Input Patches (Länge abhängig vom Input Patch)
 in VS_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
 } tcs_in[];

 // Attribute des Output Patches (Länge abhängig vom Output Patch)
 out TCS_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
 } tcs_out[];

 //uniform float TessLevelInner;
 //uniform float TessLevelOuter;
 uniform vec3 viewPos;

 /*
  * Calculates the TL for an Edge based on the distance from the 
  * Camera to the two vertices of the edge.
  */
 float GetTessLevel(float Distance0, float Distance1)
 {
 	float AvgDistance = (Distance0 + Distance1) / 2.0;
	 
 	if (AvgDistance <= 2.0) {
 		return 100.0; 
	} else if (AvgDistance <= 5.0) {
		return 50.0;
	} else {
		return 25.0;
	}
 }	 

 void main() {
	// Hier 1 zu 1 Abbildung zwischen Input und Output Kontrollpunkten
	// gl_InvocationID = contains the index of the current invocation
	tcs_out[gl_InvocationID].positionW = tcs_in[gl_InvocationID].positionW;
	tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;
	tcs_out[gl_InvocationID].texCoords = tcs_in[gl_InvocationID].texCoords;

	// Calculate the distance from the camera to the 3 control points
	float EyeToVertexDistance0 = distance(viewPos, tcs_out[0].positionW);
    float EyeToVertexDistance1 = distance(viewPos, tcs_out[1].positionW);
    float EyeToVertexDistance2 = distance(viewPos, tcs_out[2].positionW);


	// Calculate the tessellation levels
    gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
    gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
    gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
    gl_TessLevelInner[0] = gl_TessLevelOuter[2];


	// Die Tessellation Level müssen nur einem pro Patch definiert werden
	//if(gl_InvocationID == 0){
	//	gl_TessLevelInner[0] = TessLevelInner;
	//	gl_TessLevelOuter[0] = TessLevelOuter;
	//	gl_TessLevelOuter[1] = TessLevelOuter;
	//	gl_TessLevelOuter[2] = TessLevelOuter;
	//}
 }

/* Anmerkungen */
 /* Outer Subdivision:
  *		Anzahl Unterteilungen der Kante 
  *		in out Segmenten
  */

/* Inner Subdivision:
 *		Das innere Viereck wird in in-2 Segmente unterteilt 
 *		Bei in < 3 Degeneration zu Punkt
 */

/*
 * Durch die Trennung inner/outer können
 * z.B. unterschiedlich tessellierte 
 * Patches aneinandergesetzt werden, 
 * ohne daß dabei Lücken entstehen.
 */

// Fuer die Triangle Tessellation muss ein inneres und 3 aeussere Tessellation Level festgelegt werden
/*	
	gl_TessLevelInner[0] = ...;
	gl_TessLevelOuter[0] = ...;
	gl_TessLevelOuter[1] = ...;
	gl_TessLevelOuter[2] = ...;
*/


/*
* Bei Quads:
* Verwendung: 4 äußeren und 2 inneren Tessellation Level
*/

/*
* Bei Lines:
* Verwendung: 4 äußeren und 2 inneren Tessellation Level
*/


