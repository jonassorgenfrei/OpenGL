/*
 * Tessellation control shader (TCS)
 *
 * This optional stage runs once per output control point. Every invocation can
 * read the complete input patch, writes one output control point, and selects
 * distance-dependent inner and outer tessellation levels for the TES.
 */
 #version 430 core

 // tessellation levels for interpolation
 const int MIN_TESS_LEVEL = 4;
 const int MAX_TESS_LEVEL = 64;
 // distance range for interpolation
 const float MIN_DISTANCE = 20;
 const float MAX_DISTANCE = 800;

 // Define the number of control points in the output patch.
 layout(vertices=4) out;

 // Input-patch attributes; the array length equals the input patch size.
 in VS_OUT {
	vec2 texCoords;
 } tcs_in[]; // size equals number of vertices in the patch

 // Output-patch attributes; the array length equals the output patch size.
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
    // Copy each input control point to the corresponding output control point.
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

    // Tessellation levels are patch-wide, so only one invocation writes them.
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

        // Specify the tessellation levels.

         /* Outer levels control subdivision along the patch edges. */
        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        /* Inner levels control subdivision across the patch interior. */
        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);

        /* Matching outer levels on shared edges prevents cracks. */
    }
 }

/* Domain-specific tessellation-level counts */


// A triangle domain uses three outer levels and one inner level.
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


