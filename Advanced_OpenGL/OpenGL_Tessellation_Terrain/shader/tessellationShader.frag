#version 410 core

 in TES_OUT {
	float Height;
    vec3 normal;
 } fs_in;

out vec4 FragColor;

void main()
{
    float h = (fs_in.Height + 16)/64.0f;
    FragColor = vec4(h, h, h, 1.0);
    FragColor = vec4(fs_in.normal, 1.0);
}
