#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec3 aColor;

uniform vec2 offset;

out vec3 color;

void main()
{
    gl_Position = vec4(aPosition + offset, 0.0, 1.0);
    color = aColor;
}
