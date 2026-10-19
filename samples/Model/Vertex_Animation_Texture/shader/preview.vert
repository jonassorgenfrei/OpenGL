#version 330 core
out vec2 uv;
void main()
{
    // One oversized triangle covers the viewport without a separate VBO.
    uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
