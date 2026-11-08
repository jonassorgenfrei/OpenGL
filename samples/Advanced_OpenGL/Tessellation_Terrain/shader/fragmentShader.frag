#version 430 core

out vec4 FragColor;


in VS_OUT {
	float Height;
	vec3 Position;
} fs_in;

uniform int displayMode;


void main() 
{
    if (displayMode == 0) {
        float h = (fs_in.Height + 16.0) / 64.0;
        FragColor = vec4(h, h, h, 1.0);
    }
    else {
        vec3 normal = normalize(cross(dFdx(fs_in.Position), dFdy(fs_in.Position)));
        if (!gl_FrontFacing) {
            normal = -normal;
        }
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
    }
}
