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

 // tessellation levels for interpolation
 const int MIN_TESS_LEVEL = 4;
 const int MAX_TESS_LEVEL = 64;
 // distance range for interpolation
 const float MIN_DISTANCE = 20;
 const float MAX_DISTANCE = 800;

 // Definiert die Anzahl von Kontrollpunkten des Output Patches
 layout(vertices=4) out;

 // Attribute des Input Patches (Länge abhängig vom Input Patch)
 in VS_OUT {
	vec2 texCoords;
 } tcs_in[]; // size equals number of vertices in the patch

 // Attribute des Output Patches (Länge abhängig vom Output Patch)
 out TCS_OUT {
	vec2 texCoords;
 } tcs_out[];

 uniform mat4 model;
 uniform mat4 view;

 float distanceFromCamera(vec4 position) {
    // transform positions into eye/camera space
    // z is the distance from the camera
    vec4 eyeSpacePos = view * model * position;

    // "distance" from camera fitted into 0 and 1 range
    return clamp( (abs(eyeSpacePos.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
 }

 void main() {
    // Hier 1 zu 1 Abbildung zwischen Input und Output Kontrollpunkten
	// gl_InvocationID = contains the index of the current invocation
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    /** built-in GLSL var
    in gl_PerVertex
    {
        vec4 gl_Position;
        float gl_PointSize;
        float gl_ClipDistance[];
    } gl_in[gl_MaxPatchVertices];
    */
    tcs_out[gl_InvocationID].texCoords = tcs_in[gl_InvocationID].texCoords;

    // Die Tessellation Level müssen nur einem pro Patch definiert werden
    if(gl_InvocationID == 0)
    {
        // compute normalized distance for each vertex of the patch 
        float distance00 = distanceFromCamera(gl_in[0].gl_Position);
        float distance01 = distanceFromCamera(gl_in[1].gl_Position);
        float distance10 = distanceFromCamera(gl_in[2].gl_Position);
        float distance11 = distanceFromCamera(gl_in[3].gl_Position);

        // interpolate edge tessellation level abased on closer vertex
        float tessLevel0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00) );
        float tessLevel1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01) );
        float tessLevel2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11) );
        float tessLevel3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10) );

        // specify tessellation levels to perfrom

         /* Outer Subdivision:
          *		Anzahl Unterteilungen der Kante 
          *		in out Segmenten
          */
        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        /* Inner Subdivision:
         *		Das innere Viereck wird in in-2 Segmente unterteilt 
         *		Bei in < 3 Degeneration zu Punkt
         */
        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);

        /*
         * Durch die Trennung inner/outer können
         * z.B. unterschiedlich tessellierte 
         * Patches aneinandergesetzt werden, 
         * ohne daß dabei Lücken entstehen.
         */
    }
 }

/* Anmerkungen */


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
* Verwendung: 2 äußeren und keine inneren Tessellation Level
*/


