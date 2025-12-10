#version 330 core

const int KERNEL_SIZE = 64;

uniform float u_range = 50.0f;
uniform float u_near;
uniform float u_far;
uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_invProj;
uniform mat4 u_invView;
uniform sampler2D u_normal;
uniform sampler2D u_depth;
uniform vec3 u_kernel[KERNEL_SIZE];



in vec2 vUv;
out vec4 FragColor;

float linearizeDepth(float depth)
{
    // depth: [0,1] -> NDC z: [-1,1]
    float z = depth * 2.0 - 1.0;

    float n = u_near;
    float f = u_far;

    return (2.0 * n * f) / (f + n - z * (f - n));
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

mat3 makeTBN(vec3 N)
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

void makeTB(vec3 N, out vec3 T, out vec3 B)
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

vec3 jitterHemisphere(vec3 v, vec2 r, float maxAngle)
{
    // Build TBN using the original direction
    mat3 TBN = makeTBN(v);

    float phi = 2.0 * 3.14159265 * r.x;
    float cosTheta = cos(maxAngle * r.y);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 local = vec3(cos(phi) * sinTheta,
                      sin(phi) * sinTheta,
                      cosTheta);

    return normalize(TBN * local);
}

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
vec3 rand3(uint seed)
{
    uint r0 = hashU(seed);
    uint r1 = hashU(seed ^ 0x68bc21ebu);
    uint r2 = hashU(seed ^ 0x02e5be93u);

    return normalize(vec3(uf(r0), uf(r1), uf(r2)));
}
vec4 randomQuat(uint seed)
{
    float u1 = fract(sin(dot(rand2(seed), vec2(12.9898,78.233))) * 43758.5453);
    float u2 = fract(sin(dot(rand2(seed), vec2(39.3468,11.1351))) * 24634.6345);
    float u3 = fract(sin(dot(rand2(seed), vec2(73.1564,52.2356))) * 13467.9891);

    float a = sqrt(1.0 - u1);
    float b = sqrt(u1);

    float theta1 = 6.2831853 * u2; // 2¥ð
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
float sigmoid(float strength, float x, float center)
{
    return 1/(1+exp(-strength * (x - center)));
}
void main()
{   
    
    float depth   = texture(u_depth, vUv).r;
    float d = linearizeDepth(depth);
    vec3 pos = uvToWorld(vUv,depth); // we now have view space position

    vec3 normal = texture(u_normal, vUv).xyz;

    float acc = 0;
    float accFarGeom = 0;
    int hit = 0;
    float range = 100;
    float ratio = 0.05f; // keep this low. remember that the maximum range is (ratio+1)**KERNEL_SIZE
    float strength = 1.0f; //probably good as is
    float cutOff = 0.4f;
    // inverse = 1 - ratio
    // if hit == 1:
    //      range *= 1+ratio * (hit * 2 -1) (multiply by 1.02)
    // else if hit == 0:
    //      range *= 1+ratio * (hit * 2 -1) (multiply by 0.98)
    //if previous not hit: reduce range
    //if previous hit: increase range
    vec4 q = randomQuat(seed(vUv));
    int inbound = 0;
    for(int i=0;i<KERNEL_SIZE;i++)
    {
        range *= 1 + ratio * (hit * 2 -1);
        vec3 offset = rotateByQuat(u_kernel[i], q);;
        vec3 samplePos = pos + offset * range;

        vec2 sampleuv;
        float sampleDepth;
        worldToUvDepth(samplePos, sampleuv, sampleDepth);
        
        float fragPos = texture(u_depth, sampleuv).r;
        vec3 sampleNormal = texture(u_normal, sampleuv).xyz;
        float d2 = linearizeDepth(fragPos);
        //fragPos - sampleDepth is big if the neighboring geometry is much closer to the camera
        // so if fragPos-sampleDepth is big then you actually do want to add some white to this fragment
        //float rangeCheck = smoothstep(0.0, 1.0, 100 * abs(fragPos - sampleDepth)); 
        
        if(fragPos > sampleDepth)
        {
            acc += 1;
            hit = 1;
        } else
        {
            hit = 0;
        }
        //acc += max(d - d2, 0) * 0.01f;
        if(fragPos < sampleDepth) accFarGeom += (1 - smoothstep(0.0, 1.0, range / max(d - d2, 0))) * clamp(1 - dot(normal, sampleNormal), 0,1); 
        
        
    }
    //accFarGeom = sigmoid(10,accFarGeom / float(KERNEL_SIZE), 0.5);
    acc /= float(KERNEL_SIZE); //normalize with valid inbound rays only
    acc = clamp(acc,0.0,cutOff) * (1 / cutOff); //cutoff
    //accFarGeom ? 
    FragColor = vec4(vec3(acc),1.0f);
}

