#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

const int NUM_CASCADES = 4;

out vec2 TexCoords;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};


out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 LightSpacePos[NUM_CASCADES];
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));

    for (int i = 0 ; i < NUM_CASCADES ; i++) {
        vs_out.LightSpacePos[i] = lightSpaceMatrices[i] * model * vec4(aPos, 1.0);
    }

    vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}