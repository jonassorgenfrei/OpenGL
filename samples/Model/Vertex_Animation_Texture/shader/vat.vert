#version 330 core

uniform sampler2D positions;
uniform float currentFrame;
uniform bool interpolateFrames;
uniform mat4 viewProjection;

out vec3 objectPosition;
out vec2 surfaceUV;

void main()
{
    int frameCount = textureSize(positions, 0).y;
    int frame0 = int(floor(currentFrame)) % frameCount;
    int frame1 = (frame0 + 1) % frameCount;
    float blend = interpolateFrames ? fract(currentFrame) : 0.0;
    // With indexed drawing gl_VertexID is the mesh index, not the draw order.
    vec3 p0 = texelFetch(positions, ivec2(gl_VertexID, frame0), 0).rgb;
    vec3 p1 = texelFetch(positions, ivec2(gl_VertexID, frame1), 0).rgb;
    objectPosition = mix(p0, p1, blend);
    surfaceUV = objectPosition.xy / vec2(4.0, 2.4) + 0.5;
    gl_Position = viewProjection * vec4(objectPosition, 1.0);
}
