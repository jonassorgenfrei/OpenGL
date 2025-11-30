#version 330

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(Position, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = mat3(transpose(inverse(model))) * Normal;
    gl_Position = projection * view * worldPos;
}
