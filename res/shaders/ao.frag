#version 330 core

const int KERNEL_SIZE = 64;

uniform float u_near;
uniform float u_far;
uniform float u_radius = 96.0f;  // world units

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_invProj;
uniform mat4 u_invView;
uniform sampler2D u_depth;
uniform sampler2D u_normal; //used to test whether there is an object here. 4th float is cleared to 0 if there is none
uniform vec3 u_kernel[KERNEL_SIZE];

in vec2 vUv;
out vec4 FragColor;

uint hashU(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}
float uf(uint x) {
    return float(x) * (1.0 / 4294967296.0);
}
uint seed(vec2 uv)
{
    return floatBitsToUint(uv.x * 4096.0) ^ (floatBitsToUint(uv.y * 4096.0)*1664525u) ^ 1013904223u;
}
vec2 rand2(uint seed)
{
    uint r1 = hashU(seed ^ 0x68bc21ebu);
    uint r2 = hashU(seed ^ 0x02e5be93u);
    return normalize(vec2(uf(r1), uf(r2)));
}
vec3 uvToWorld(vec2 uv, float depth)
{
    // depth is in [0,1] non-linear from depth buffer
    float z = depth * 2.0 - 1.0;               // NDC depth

    vec4 ndc = vec4(uv * 2.0 - 1.0, z, 1.0);

    // �� view space
    vec4 view = u_invProj * ndc;
    view /= view.w;

    // �� world space
    vec4 world = u_invView * view;
    return world.xyz;
}
void worldToUvDepth(vec3 worldPos, out vec2 uv, out float depth)
{
    // �� clip space
    vec4 clip = u_proj * u_view * vec4(worldPos, 1.0);

    // perspective divide �� NDC
    vec3 ndc = clip.xyz / clip.w;   // [-1, 1]

    // NDC �� uv
    uv = ndc.xy * 0.5 + 0.5;        // [0, 1]

    // NDC z �� depth buffer
    depth = (ndc.z + 1.0) * 0.5;    // [0, 1]
}
vec4 randomQuat(uint seed)
{
    float u1 = fract(sin(dot(rand2(seed), vec2(12.9898,78.233))) * 43758.5453);
    float u2 = fract(sin(dot(rand2(seed), vec2(39.3468,11.1351))) * 24634.6345);
    float u3 = fract(sin(dot(rand2(seed), vec2(73.1564,52.2356))) * 13467.9891);

    float a = sqrt(1.0 - u1);
    float b = sqrt(u1);

    float theta1 = 6.2831853 * u2; // 2pi
    float theta2 = 6.2831853 * u3;

    return vec4(
        a * sin(theta1),   // x
        a * cos(theta1),   // y
        b * sin(theta2),   // z
        b * cos(theta2)    // w
    );
}
vec3 rotateByQuat(vec3 v, vec4 q)
{
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}
float sigmoid(float x, float k, float offset)
{
    return 1.0 / (1.0 + exp(-k * (x - offset)));
}
float linearViewZ(float depth, float near, float far)
{
    float ndc = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - ndc * (far - near));
}

void main()
{   
    vec4 normal = texture(u_normal, vUv);

    if(normal.w < 0.5f)
    {
        FragColor = vec4(1.0f, 1.0f, 1.0f, 0.0f);
        return;
    }
    float depth   = texture(u_depth, vUv).r;
    vec3 pos = uvToWorld(vUv,depth);

    vec4 q = randomQuat(seed(vUv));

    vec3 n = normalize(normal.xyz);
    float originZ = linearViewZ(depth, u_near, u_far);
    float occlusion = 0.0;
    for(int i=0;i<KERNEL_SIZE;i++)
    {
        vec3 offset = rotateByQuat(u_kernel[i], q);
        if(dot(offset, n) < 0.0) offset = -offset; //flip into the hemisphere so that no sample is wasted

        float s = float(i + 1) / float(KERNEL_SIZE);
        vec3 samplePos = pos + offset * u_radius * (0.1 + 0.9 * s * s);

        vec2 sampleUV;
        float sampleDepth;
        worldToUvDepth(samplePos, sampleUV, sampleDepth);

        float sampleZ  = linearViewZ(sampleDepth, u_near, u_far);
        float surfaceZ = linearViewZ(texture(u_depth, sampleUV).r, u_near, u_far);
        float bias = 2.0 + 1.7e-8 * sampleZ * sampleZ; //depth buffer precision falls with distance squared
        float inRange = smoothstep(0.0, 1.0, u_radius / max(abs(originZ - surfaceZ), 0.001));
        if(surfaceZ < sampleZ - bias) occlusion += inRange;
    }
    //contrast is applied in deferred.frag, after the blur
    FragColor = vec4(vec3(1.0 - occlusion / float(KERNEL_SIZE)), 1.0f);
}