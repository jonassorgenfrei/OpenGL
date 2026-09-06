/*
 * Tessellation control shader (TCS)
 *
 * This optional stage runs once per output control point. Every invocation can
 * read the complete input patch, writes one output control point, and can help
 * choose the patch's inner and outer tessellation levels for the TES.
 */
 #version 430 core

 // Define the number of control points in the output patch.
 layout (vertices = 3) out;

 // Input-patch attributes; the array length equals the input patch size.
 in VS_OUT {
	vec3 positionW;
	vec3 normal;
	vec2 texCoords;
 } tcs_in[];

 // Output-patch attributes; the array length equals the output patch size.
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
	// Copy each input control point to the corresponding output control point.
	// gl_InvocationID = contains the index of the current invocation
	tcs_out[gl_InvocationID].positionW = tcs_in[gl_InvocationID].positionW;
	tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;
	tcs_out[gl_InvocationID].texCoords = tcs_in[gl_InvocationID].texCoords;

	// Calculate the distance from the camera to the 3 control points
	float EyeToVertexDistance0 = distance(viewPos, tcs_out[0].positionW);
    float EyeToVertexDistance1 = distance(viewPos, tcs_out[1].positionW);
    float EyeToVertexDistance2 = distance(viewPos, tcs_out[2].positionW);

	// Tessellation levels are patch-wide, so only one invocation writes them.
	// Calculate the tessellation levels
	if(gl_InvocationID == 0){
		gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
		gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
		gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
		gl_TessLevelInner[0] = gl_TessLevelOuter[2];
	}
 }

/*
 * Outer levels control subdivision along patch edges; inner levels control the
 * patch interior. Adjacent patches avoid cracks when their shared outer edge
 * uses the same level. A triangle domain has three outer levels and one inner
 * level.
 */
/*	
	gl_TessLevelInner[0] = ...;
	gl_TessLevelOuter[0] = ...;
	gl_TessLevelOuter[1] = ...;
	gl_TessLevelOuter[2] = ...;
*/


/*
* A quad domain uses four outer and two inner tessellation levels.
*/

/*
* An isoline domain uses two outer levels and no inner levels.
*/


