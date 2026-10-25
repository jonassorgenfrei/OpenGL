#version 330 core

in vec2 gaussianPosition;
flat in vec4 color;

uniform bool pointMode;

out vec4 FragColor;

void main()
{
    if (pointMode)
    {
        // GL_POINTS are square by default. Discard their corners so the raw
        // samples remain visually distinct from square raster pixels.
        vec2 pointPosition = gl_PointCoord * 2.0 - 1.0;
        if (dot(pointPosition, pointPosition) > 1.0)
            discard;
        FragColor = vec4(color.rgb, 1.0);
        return;
    }

    // In the standardized ellipse frame, the 2D Gaussian density simplifies
    // to exp(-0.5 * r^2); no inverse covariance is needed per fragment.
    float gaussian = exp(-0.5 * dot(gaussianPosition, gaussianPosition));
    float alpha = color.a * gaussian;
    // Contributions below one 8-bit alpha step are imperceptible and would
    // only add blending work.
    if (alpha < (1.0 / 255.0))
        discard;

    // main.cpp uses GL_ONE, GL_ONE_MINUS_SRC_ALPHA, so RGB must be
    // premultiplied. This composes the CPU-sorted splats back-to-front.
    FragColor = vec4(color.rgb * alpha, alpha);
}
