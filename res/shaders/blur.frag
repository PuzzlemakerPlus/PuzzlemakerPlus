#version 330 core

uniform sampler2D u_input;     // input texture
uniform vec2 u_direction;      // (1,0) for horizontal, (0,1) for vertical
uniform int u_radius = 10;
uniform float u_sigma = 3.0f;
uniform sampler2D u_depth;
uniform sampler2D u_normal;
uniform float u_near;
uniform float u_far;
out vec4 FragColor;
in vec2 vUv;

float linZ(float depth)
{
    float ndc = depth * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near - ndc * (u_far - u_near));
}

void main()
{
    vec4 init = texture(u_input, vUv);
    float blur = init.w;
    // Skip blur if this pixel isn't marked for it
    if (blur < 0.5f)
    {
        FragColor = init;
        return;
    }

    ivec2 texSize = textureSize(u_input, 0);
    vec2 texel = 1.0 / vec2(texSize);

    int radius = u_radius;
    if (radius < 1) radius = 1;

    float sigma = u_sigma;
    if (sigma <= 0.0)
        sigma = float(radius) * 0.5 + 0.0001; // avoid div by zero

    float twoSigma2 = 2.0 * sigma * sigma;

    vec3 n0 = texture(u_normal, vUv).xyz;
    float z0 = linZ(texture(u_depth, vUv).r);

    vec4 sum      = vec4(0.0);
    float weightSum = 0.0;

    for (int oy = -radius; oy <= radius; ++oy)
    {
        for (int ox = -radius; ox <= radius; ++ox)
        {
            vec2 offset = vec2(ox, oy) * texel;
            vec2 uv = vUv + offset;
            vec4 s = texture(u_input, uv);
            bool sameFace = dot(texture(u_normal, uv).xyz, n0) > 0.9;
            bool sameDepth = abs(linZ(texture(u_depth, uv).r) - z0) < 16.0 + 0.004 * z0;

            // Only mix with neighbors that are also ¡°blur-eligible¡±
            if (s.w > 0.5f && sameFace && sameDepth)
            {
                float r2 = float(ox * ox + oy * oy); // distance^2
                float w  = exp(-r2 / twoSigma2);     // Gaussian weight

                sum      += s * w;
                weightSum += w;
            }
        }
    }

    if (weightSum > 0.0)
        FragColor = sum / weightSum;
    else
        FragColor = init; // fallback if no valid neighbors
}