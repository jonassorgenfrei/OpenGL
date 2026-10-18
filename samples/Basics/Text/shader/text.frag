#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;
uniform float alpha;
uniform bool textured;

void main()
{    
    vec4 sampled = textured ? vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r) : vec4(1.0); // sample glyph or draw a solid rectangle
    color = vec4(textColor, alpha) * sampled; // adjusting font color multiplying RGB
}
