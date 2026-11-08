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

 uniform vec3 viewPos;
 uniform float tessellationMultiplier;

 /*
  * Calculates the TL for an Edge based on the distance from the 
  * Camera to the two vertices of the edge.
  */
 float GetTessLevel(float Distance0, float Distance1)
 {
 	float AvgDistance = (Distance0 + Distance1) / 2.0;
	 
 	if (AvgDistance <= 2.0) {
		return 64.0;
	} else if (AvgDistance <= 5.0) {
		return 50.0;
	} else {
		return 25.0;
	}
 }	 

 float ScaledTessLevel(float Distance0, float Distance1)
 {
	// Vertex displacement cannot reproduce the height map when a patch collapses
	// to only a handful of generated vertices. Keep enough subdivisions to
	// preserve the displaced shape while still allowing a large quality range.
	return clamp(GetTessLevel(Distance0, Distance1) * tessellationMultiplier, 8.0, 64.0);
 }

 void main() {
	// Copy each input control point to the corresponding output control point.
	// gl_InvocationID = contains the index of the current invocation
	tcs_out[gl_InvocationID].positionW = tcs_in[gl_InvocationID].positionW;
	tcs_out[gl_InvocationID].normal = tcs_in[gl_InvocationID].normal;
	tcs_out[gl_InvocationID].texCoords = tcs_in[gl_InvocationID].texCoords;

	// Tessellation levels are patch-wide, so only one invocation writes them.
	if(gl_InvocationID == 0){
		float EyeToVertexDistance0 = distance(viewPos, tcs_in[0].positionW);
		float EyeToVertexDistance1 = distance(viewPos, tcs_in[1].positionW);
		float EyeToVertexDistance2 = distance(viewPos, tcs_in[2].positionW);

		gl_TessLevelOuter[0] = ScaledTessLevel(EyeToVertexDistance1, EyeToVertexDistance2);
		gl_TessLevelOuter[1] = ScaledTessLevel(EyeToVertexDistance2, EyeToVertexDistance0);
		gl_TessLevelOuter[2] = ScaledTessLevel(EyeToVertexDistance0, EyeToVertexDistance1);
		gl_TessLevelInner[0] = max(gl_TessLevelOuter[0],
			max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));
	}
 }

/*
 * Outer levels control subdivision along patch edges; inner levels control the
 * patch interior. Adjacent patches avoid cracks when their shared outer edge
 * uses the same level. A triangle domain has three outer levels and one inner
 * level.
 */

