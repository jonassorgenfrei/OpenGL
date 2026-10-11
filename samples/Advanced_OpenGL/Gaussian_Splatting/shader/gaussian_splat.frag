#version 330 core

in vec2 gaussianPosition;
in vec4 color;

uniform bool pointMode;

out vec4 FragColor;

void main()
{
    if (pointMode)
    {
        vec2 pointPosition = gl_PointCoord * 2.0 - 1.0;
        if (dot(pointPosition, pointPosition) > 1.0)
            discard;
        FragColor = vec4(color.rgb, 1.0);
        return;
    }

    float gaussian = exp(-0.5 * dot(gaussianPosition, gaussianPosition));
    float alpha = color.a * gaussian;
    if (alpha < (1.0 / 255.0))
        discard;

    // Premultiplied alpha avoids dark fringes where splats overlap.
    FragColor = vec4(color.rgb * alpha, alpha);
}
