#version 410 core

 in TES_OUT {
	float Height;
    vec3 normal;
 } fs_in;

out vec4 FragColor;
uniform int displayMode;

void main()
{
    if (displayMode == 0) {
        float h = (fs_in.Height + 16.0) / 64.0;
        FragColor = vec4(h, h, h, 1.0);
    }
    else {
        FragColor = vec4(fs_in.normal * 0.5 + 0.5, 1.0);
    }
}
