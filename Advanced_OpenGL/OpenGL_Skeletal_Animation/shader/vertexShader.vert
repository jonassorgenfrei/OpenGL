#version 430 core

layout (location = 0) in vec3 aPos;

out VS_OUT {
	float Height;
	vec3 Position;
} vs_out;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main() {
    vs_out.Height = aPos.y;
    vs_out.Position = (view * model * vec4(aPos, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}