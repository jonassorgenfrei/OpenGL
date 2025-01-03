#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec3 Pos;
out vec2 TexCoord;
out vec3 Color;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	Normal = mat3(transpose(inverse(model))) * aNormal;
	Pos = (model * vec4(aPos, 1.0f)).xyz;
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
	Color = aColor;
}