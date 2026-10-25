#version 330 core

layout (location = 0) in vec2 aPosition;

uniform vec2 offset;
uniform vec2 scale;
uniform float depth;

void main()
{
    gl_Position = vec4(aPosition * scale + offset, depth, 1.0);
}
