#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D containerTexture;
uniform sampler2D faceTexture;
uniform float mixValue;

void main()
{
    fragColor = mix(texture(containerTexture, texCoord),
                    texture(faceTexture, texCoord), mixValue);
}
