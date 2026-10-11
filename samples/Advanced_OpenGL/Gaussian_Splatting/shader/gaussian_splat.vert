#version 330 core

layout (location = 0) in vec2 quadPosition;
layout (location = 1) in vec3 splatCenter;
layout (location = 2) in vec3 splatLogScale;
layout (location = 3) in vec4 splatRotation; // w, x, y, z
layout (location = 4) in float splatOpacityLogit;
layout (location = 5) in uint splatShIndex;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform bool pointMode;
uniform vec3 cameraPosition;
uniform vec2 viewportSize;
uniform samplerBuffer shData;

out vec2 gaussianPosition;
out vec4 color;

float shCoefficient(uint coefficient)
{
    uint scalarIndex = splatShIndex * 48u + coefficient;
    vec4 values = texelFetch(shData, int(scalarIndex / 4u));
    return values[int(scalarIndex % 4u)];
}

vec3 shVector(uint order)
{
    if (order == 0u)
        return vec3(shCoefficient(0u), shCoefficient(1u), shCoefficient(2u));

    uint coefficient = order - 1u;
    return vec3(shCoefficient(3u + coefficient),
                shCoefficient(18u + coefficient),
                shCoefficient(33u + coefficient));
}

vec3 evaluateSphericalHarmonics(vec3 direction)
{
    const float C0 = 0.28209479177387814;
    const float C1 = 0.4886025119029199;
    const float C2_0 = 1.0925484305920792;
    const float C2_1 = -1.0925484305920792;
    const float C2_2 = 0.31539156525252005;
    const float C2_3 = -1.0925484305920792;
    const float C2_4 = 0.5462742152960396;
    const float C3_0 = -0.5900435899266435;
    const float C3_1 = 2.890611442640554;
    const float C3_2 = -0.4570457994644658;
    const float C3_3 = 0.3731763325901154;
    const float C3_4 = -0.4570457994644658;
    const float C3_5 = 1.445305721320277;
    const float C3_6 = -0.5900435899266435;

    float x = direction.x;
    float y = direction.y;
    float z = direction.z;
    float xx = x * x;
    float yy = y * y;
    float zz = z * z;

    vec3 result = C0 * shVector(0u);
    result += -C1 * y * shVector(1u) + C1 * z * shVector(2u) - C1 * x * shVector(3u);
    result += C2_0 * x * y * shVector(4u);
    result += C2_1 * y * z * shVector(5u);
    result += C2_2 * (2.0 * zz - xx - yy) * shVector(6u);
    result += C2_3 * x * z * shVector(7u);
    result += C2_4 * (xx - yy) * shVector(8u);
    result += C3_0 * y * (3.0 * xx - yy) * shVector(9u);
    result += C3_1 * x * y * z * shVector(10u);
    result += C3_2 * y * (4.0 * zz - xx - yy) * shVector(11u);
    result += C3_3 * z * (2.0 * zz - 3.0 * xx - 3.0 * yy) * shVector(12u);
    result += C3_4 * x * (4.0 * zz - xx - yy) * shVector(13u);
    result += C3_5 * z * (xx - yy) * shVector(14u);
    result += C3_6 * x * (xx - 3.0 * yy) * shVector(15u);
    return max(result + 0.5, vec3(0.0));
}

mat3 quaternionRotation(vec4 quaternion)
{
    vec4 q = normalize(quaternion);
    float w = q.x;
    float x = q.y;
    float y = q.z;
    float z = q.w;
    return mat3(
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y + w * z),       2.0 * (x * z - w * y),
        2.0 * (x * y - w * z),       1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z + w * x),
        2.0 * (x * z + w * y),       2.0 * (y * z - w * x),       1.0 - 2.0 * (x * x + y * y));
}

void main()
{
    mat3 modelRotation = mat3(model);
    vec3 worldCenter = (model * vec4(splatCenter, 1.0)).xyz;
    vec4 centerInView = view * vec4(worldCenter, 1.0);

    // SH coefficients are defined in the PLY's coordinate system. Transform
    // the view direction back into that system before evaluating the basis.
    vec3 worldViewDirection = normalize(worldCenter - cameraPosition);
    vec3 splatViewDirection = transpose(modelRotation) * worldViewDirection;
    color = vec4(evaluateSphericalHarmonics(splatViewDirection),
                 1.0 / (1.0 + exp(-splatOpacityLogit)));

    if (centerInView.z >= -0.1)
    {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        gl_PointSize = 0.0;
        gaussianPosition = vec2(0.0);
        return;
    }

    if (pointMode)
    {
        gl_Position = projection * centerInView;
        gl_PointSize = 2.0;
        gaussianPosition = vec2(0.0);
        return;
    }

    mat3 rotation = quaternionRotation(splatRotation);
    vec3 scale = exp(splatLogScale);
    mat3 scaleSquared = mat3(scale.x * scale.x, 0.0, 0.0,
                             0.0, scale.y * scale.y, 0.0,
                             0.0, 0.0, scale.z * scale.z);
    mat3 localCovariance = rotation * scaleSquared * transpose(rotation);
    mat3 covariance = modelRotation * localCovariance * transpose(modelRotation);
    mat3 viewRotation = mat3(view);
    mat3 viewCovariance = viewRotation * covariance * transpose(viewRotation);

    float inverseDepth = -1.0 / centerInView.z;
    float inverseDepthSquared = inverseDepth * inverseDepth;
    vec3 jacobianX = vec3(projection[0][0] * inverseDepth, 0.0,
                          projection[0][0] * centerInView.x * inverseDepthSquared);
    vec3 jacobianY = vec3(0.0, projection[1][1] * inverseDepth,
                          projection[1][1] * centerInView.y * inverseDepthSquared);

    float a = dot(jacobianX, viewCovariance * jacobianX);
    float b = dot(jacobianX, viewCovariance * jacobianY);
    float d = dot(jacobianY, viewCovariance * jacobianY);
    // Guarantee at least a half-pixel footprint on either screen axis.
    a += 1.0 / (viewportSize.x * viewportSize.x);
    d += 1.0 / (viewportSize.y * viewportSize.y);

    float midpoint = 0.5 * (a + d);
    float radius = sqrt(max(0.0, 0.25 * (a - d) * (a - d) + b * b));
    float lambda1 = max(midpoint + radius, 1e-8);
    float lambda2 = max(midpoint - radius, 1e-8);
    vec2 axis1 = abs(b) > 1e-8
        ? normalize(vec2(b, lambda1 - a))
        : (a >= d ? vec2(1.0, 0.0) : vec2(0.0, 1.0));
    vec2 axis2 = vec2(-axis1.y, axis1.x);
    vec2 ndcOffset = 3.0 * (quadPosition.x * sqrt(lambda1) * axis1 +
                            quadPosition.y * sqrt(lambda2) * axis2);

    gl_Position = projection * centerInView;
    gl_Position.xy += ndcOffset * gl_Position.w;

    gaussianPosition = quadPosition * 3.0;
}
