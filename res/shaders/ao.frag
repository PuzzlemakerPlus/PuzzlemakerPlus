#version 330 core

const int KERNEL_SIZE = 64;

uniform float u_range = 2.0f;
uniform float u_variableRange = 0.05f;  // keep this low. remember that the maximum range is (ratio+1)**KERNEL_SIZE

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

    // ¡æ view space
    vec4 view = u_invProj * ndc;
    view /= view.w;

    // ¡æ world space
    vec4 world = u_invView * view;
    return world.xyz;
}
void worldToUvDepth(vec3 worldPos, out vec2 uv, out float depth)
{
    // ¡æ clip space
    vec4 clip = u_proj * u_view * vec4(worldPos, 1.0);

    // perspective divide ¡æ NDC
    vec3 ndc = clip.xyz / clip.w;   // [-1, 1]

    // NDC ¡æ uv
    uv = ndc.xy * 0.5 + 0.5;        // [0, 1]

    // NDC z ¡æ depth buffer
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

    int hit = 0;
    float range = u_range;
    float ratio = u_variableRange;
    
    vec4 q = randomQuat(seed(vUv));

    float occlusion = 0;
    int count = 0;
    for(int i=0;i<KERNEL_SIZE;i++)
    {
        range *= 1 + ratio;
        vec3 offset = rotateByQuat(u_kernel[i], q);
        if(dot(normalize(offset), normalize(normal.xyz)) < 0) continue; //only check with hemisphere
        count++;
        vec3 samplePos = pos + offset * range;

        vec2 sampleUV;
        float sampleDepth;
        worldToUvDepth(samplePos, sampleUV, sampleDepth);
        float fragPos = texture(u_depth, sampleUV).r;
        float linSampleZ = linearViewZ(sampleDepth, 1, 1000);
        float linFragZ   = linearViewZ(fragPos, 1, 1000);
        float depthDiff = linSampleZ - linFragZ;
        if(0.0 < depthDiff && depthDiff < 1)
        {
            occlusion += 1;
        }
    }
    occlusion = 1 - occlusion / float(count);
    occlusion = clamp(occlusion * 2 - 1, 0,1);
    occlusion = sigmoid(occlusion, 10, 0.7);
    FragColor = vec4(vec3(occlusion),1.0f);
}