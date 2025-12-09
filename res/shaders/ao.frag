#version 330 core

in vec2 vUv;
out vec4 FragAO;

uniform sampler2D u_normal;
uniform sampler2D u_depth;

uniform mat4 u_view;
uniform mat4 u_invView;
uniform mat4 u_proj;
uniform mat4 u_invProj;
uniform float u_near;
uniform float u_far;

// SSAO params
uniform float u_radius = 1.0f;
uniform float u_bias = 0.025f;
uniform float u_intensity = 1.0f;
uniform float u_maxDistance = 100.0f;
uniform uint  u_frame = 0u;

const int SAMPLES = 16;

// Reuse your existing function
vec3 getViewPos(vec2 uv, float depth)
{
    float z_ndc = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, z_ndc, 1.0);
    vec4 viewPos = u_invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

// New: get world-space position
vec3 getWorldPos(vec2 uv, float depth)
{
    vec3 viewPos = getViewPos(uv, depth);
    vec4 worldPos = u_invView * vec4(viewPos, 1.0);
    return worldPos.xyz;
}

// ------------------------------------------------------------
// GPU random number generator (ASCII only)
// ------------------------------------------------------------
uint hash_uvec3(uvec3 x)
{
    x = x * 1664525u + 1013904223u;
    x.x += x.y * x.z;
    x.y += x.z * x.x;
    x.z += x.x * x.y;
    x ^= (x >> 16);
    x += (x << 3);
    x ^= (x >> 4);
    x *= 0x27d4eb2du;
    x ^= (x >> 15);
    return x.x ^ x.y ^ x.z;
}

float randFloat(uvec3 seed)
{
    return float(hash_uvec3(seed)) / 4294967295.0;
}

// ------------------------------------------------------------
// Generate hemisphere sample in tangent space
// ------------------------------------------------------------
vec3 hemisphereSample(uint baseSeed, int index)
{
    uvec3 seed1 = uvec3(gl_FragCoord.xy, baseSeed + uint(index) * 2u);
    uvec3 seed2 = uvec3(gl_FragCoord.yx, baseSeed + uint(index) * 2u + 1u);

    float xi1 = randFloat(seed1);
    float xi2 = randFloat(seed2);

    float phi = 6.2831853 * xi1; // 2 * PI
    float cosTheta = xi2;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;

    return vec3(x, y, z);
}
vec3 hemisphereSample(uvec3 seed)
{
    float xi1 = randFloat(seed); // 0..1
    float xi2 = randFloat(seed); // 0..1

    float phi = 6.2831853 * xi1;    // 2 * PI
    float cosTheta = xi2;           // 0..1 (hemisphere)
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;             // always >= 0

    return vec3(x, y, z);           // hemisphere around +Z
}

void buildTBN(in vec3 N, out vec3 T, out vec3 B)
{
    vec3 up = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0)
                                 : vec3(0.0, 1.0, 0.0);

    T = normalize(cross(up, N));
    B = cross(N, T);
}

// ------------------------------------------------------------
// Main SSAO shader
// ------------------------------------------------------------
void main()
{
    float depth = texture(u_depth, vUv).r;
    if (depth >= 1.0)
    {
        FragAO = vec4(1.0);
        return;
    }
    vec3 worldPos = getWorldPos(vUv, depth);
    // world normal because u_normal is world normal
    vec3 N = normalize(texture(u_normal, vUv).xyz);
    vec3 fragPos = getViewPos(vUv, depth);

    vec3 T;
    vec3 B;
    buildTBN(N, T, B); // create axis
    float occlusion = 0.0;
    for (int i = 0; i < SAMPLES; ++i)
    {
        uvec3 seed = uvec3(gl_FragCoord.xy, uint(i));
        vec3 sampleLocal = hemisphereSample(seed);     // tangent space
        vec3 samplePos =
            T * sampleLocal.x +
            B * sampleLocal.y +
            N * sampleLocal.z;  // hemisphere around N in world space
        samplePos = fragPos + samplePos * u_radius; 
        vec4 offset = vec4(samplePos, 1.0);
        offset      = u_proj * offset;    // from view to clip-space
        offset.xyz /= offset.w;               // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        float sampleDepth = texture(u_depth, offset.xy).r;
        occlusion += (sampleDepth >= samplePos.z + u_bias ? 1.0 : 0.0); 
        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(fragPos.z - sampleDepth));
        occlusion       += (sampleDepth >= samplePos.z + u_bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / SAMPLES);
    FragAO = vec4(vec3(occlusion),1.0f);  
}
