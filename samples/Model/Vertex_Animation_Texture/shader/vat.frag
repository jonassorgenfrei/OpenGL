#version 330 core

in vec3 objectPosition;
in vec2 surfaceUV;
out vec4 FragColor;

void main()
{
    // Reconstruct a deformed face normal, avoiding a second VAT for normals.
    vec3 normal = normalize(cross(dFdx(objectPosition), dFdy(objectPosition)));
    float diffuse = abs(dot(normal, normalize(vec3(0.3, 0.7, 1.0))));
    vec2 cell = floor(surfaceUV * vec2(16.0, 8.0));
    float checker = mod(cell.x + cell.y, 2.0);
    vec3 color = mix(vec3(0.08, 0.38, 0.65), vec3(0.18, 0.75, 0.85), checker);
    FragColor = vec4(color * (0.3 + 0.7 * diffuse), 1.0);
}
