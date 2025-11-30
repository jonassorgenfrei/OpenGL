#version 330
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} fs_in;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 baseColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform int ambientOnly;

void main()
{
    vec3 norm = normalize(fs_in.Normal);
    vec3 ambient = ambientStrength * baseColor;

    if (ambientOnly == 1) {
        FragColor = vec4(ambient, 1.0);
        return;
    }

    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * baseColor;

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
