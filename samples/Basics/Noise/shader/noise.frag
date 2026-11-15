#version 330 core

in vec2 texCoords;
out vec4 fragColor;

uniform vec2 resolution;
uniform float time;
uniform float scale;
uniform int noiseType;

// Quintic interpolation reaches zero first and second derivatives at both ends.
vec2 fade(vec2 value)
{
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

vec3 fade(vec3 value)
{
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

float hash12(vec2 point)
{
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 gradient2(vec2 point)
{
    float angle = 6.28318530718 * hash12(point);
    return vec2(cos(angle), sin(angle));
}

vec3 gradient3(vec3 point)
{
    vec3 value = fract(sin(vec3(
        dot(point, vec3(127.1, 311.7,  74.7)),
        dot(point, vec3(269.5, 183.3, 246.1)),
        dot(point, vec3(113.5, 271.9, 124.6)))) * 43758.5453123);
    return normalize(value * 2.0 - 1.0);
}

float valueNoise(vec2 point)
{
    vec2 cell = floor(point);
    vec2 local = fract(point);
    vec2 weight = fade(local);

    float bottom = mix(hash12(cell), hash12(cell + vec2(1.0, 0.0)), weight.x);
    float top = mix(hash12(cell + vec2(0.0, 1.0)), hash12(cell + vec2(1.0, 1.0)), weight.x);
    return mix(bottom, top, weight.y);
}

float perlinNoise(vec2 point)
{
    vec2 cell = floor(point);
    vec2 local = fract(point);
    vec2 weight = fade(local);

    float n00 = dot(gradient2(cell), local);
    float n10 = dot(gradient2(cell + vec2(1.0, 0.0)), local - vec2(1.0, 0.0));
    float n01 = dot(gradient2(cell + vec2(0.0, 1.0)), local - vec2(0.0, 1.0));
    float n11 = dot(gradient2(cell + vec2(1.0, 1.0)), local - vec2(1.0, 1.0));
    return mix(mix(n00, n10, weight.x), mix(n01, n11, weight.x), weight.y) * 0.5 + 0.5;
}

float perlinNoise(vec3 point)
{
    vec3 cell = floor(point);
    vec3 local = fract(point);
    vec3 weight = fade(local);

    float n000 = dot(gradient3(cell), local);
    float n100 = dot(gradient3(cell + vec3(1.0, 0.0, 0.0)), local - vec3(1.0, 0.0, 0.0));
    float n010 = dot(gradient3(cell + vec3(0.0, 1.0, 0.0)), local - vec3(0.0, 1.0, 0.0));
    float n110 = dot(gradient3(cell + vec3(1.0, 1.0, 0.0)), local - vec3(1.0, 1.0, 0.0));
    float n001 = dot(gradient3(cell + vec3(0.0, 0.0, 1.0)), local - vec3(0.0, 0.0, 1.0));
    float n101 = dot(gradient3(cell + vec3(1.0, 0.0, 1.0)), local - vec3(1.0, 0.0, 1.0));
    float n011 = dot(gradient3(cell + vec3(0.0, 1.0, 1.0)), local - vec3(0.0, 1.0, 1.0));
    float n111 = dot(gradient3(cell + vec3(1.0, 1.0, 1.0)), local - vec3(1.0, 1.0, 1.0));

    float x00 = mix(n000, n100, weight.x);
    float x10 = mix(n010, n110, weight.x);
    float x01 = mix(n001, n101, weight.x);
    float x11 = mix(n011, n111, weight.x);
    return mix(mix(x00, x10, weight.y), mix(x01, x11, weight.y), weight.z) * 0.5 + 0.5;
}

float fractalPerlin(vec2 point)
{
    float sum = 0.0;
    float amplitude = 0.5;
    float normalization = 0.0;
    for (int octave = 0; octave < 5; ++octave)
    {
        sum += amplitude * perlinNoise(point);
        normalization += amplitude;
        point = point * 2.03 + vec2(17.1, 9.2);
        amplitude *= 0.5;
    }
    return sum / normalization;
}

float animatedPerlin(vec2 point, float animationTime)
{
    float sum = 0.0;
    float amplitude = 0.5;
    float normalization = 0.0;
    for (int octave = 0; octave < 4; ++octave)
    {
        // Time is a third coordinate, so the pattern evolves instead of sliding.
        sum += amplitude * perlinNoise(vec3(point, animationTime));
        normalization += amplitude;
        point = point * 2.03 + vec2(17.1, 9.2);
        animationTime *= 1.37;
        amplitude *= 0.5;
    }
    return sum / normalization;
}

void main()
{
    vec2 centered = texCoords - 0.5;
    centered.x *= resolution.x / max(resolution.y, 1.0);
    vec2 point = centered * scale;

    float noiseValue;
    if (noiseType == 0)
        noiseValue = valueNoise(point);
    else if (noiseType == 1)
        noiseValue = perlinNoise(point);
    else if (noiseType == 2)
        noiseValue = fractalPerlin(point);
    else
        noiseValue = animatedPerlin(point, time * 0.35);

    fragColor = vec4(vec3(noiseValue), 1.0);
}
