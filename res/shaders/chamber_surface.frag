#version 330 core

in vec4 vertexColor;
in float v_viewZ;
in vec3 vWorldPos;  // world position
in vec3 vNormal;    // world normal

flat in uint vSurfaceId;
flat in uint vParentId;
flat in uint vWallSide;

uniform vec3 u_fogColor;
uniform float u_fogStart;
uniform float u_fogEnd;
uniform int u_debugView;

uniform int u_drawShadow;
uniform sampler2D u_ShadowMap;
uniform mat4 u_lightVP;
uniform vec3 u_lightDir;

out vec4 FragColor;

// --------- Shadow helper ---------
float computeShadowFactor(vec3 worldPos, vec3 normal)
{
    // Transform to light clip space
    vec4 lightSpacePos = u_lightVP * vec4(worldPos, 1.0);
    vec3 projCoords    = lightSpacePos.xyz / lightSpacePos.w;

    // NDC [-1,1] -> [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // Outside the shadow map -> treat as lit
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
    {
        return 1.0;
    }

    float currentDepth = projCoords.z;

    // Bias to reduce acne (depends on angle to light)
    vec3  N      = normalize(normal);
    vec3  L      = normalize(u_lightDir);           // from fragment to light
    float ndotl  = max(dot(N, L), 0.0);
    float bias   = max(0.0005 * (1.0 - ndotl), 0.00);

    // PCF sample
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(u_ShadowMap, projCoords.xy + offset).r;
            shadow += (currentDepth - bias > closestDepth) ? 0.0 : 1.0;
        }
    }

    shadow /= 9.0;

    return shadow;
}


void main()
{
    vec3 diffuse;

    float fog = clamp((v_viewZ - u_fogStart) / (u_fogEnd - u_fogStart), 0.0, 1.0);

    if (u_debugView == 1) {
        // ID debug view: no lighting / shadow, just raw color
        uint key = vParentId ^ (vSurfaceId << 1) ^ (vWallSide << 5);
        key ^= key >> 16; key *= 0x7feb352du;
        key ^= key >> 15; key *= 0x846ca68bu;
        key ^= key >> 16;
        diffuse = vec3( key        & 255u,
                        (key >> 8)  & 255u,
                        (key >> 16) & 255u) / 255.0;

        vec3 finalColorDbg = mix(diffuse, u_fogColor, fog);
        FragColor = vec4(finalColorDbg, 1.0);
        return;
    }
    else
    {
        diffuse = vertexColor.rgb;
    }
    vec3 color;
    if(u_drawShadow == 1)
    {
        // ----- Lighting + shadow -----
        vec3 N = normalize(vNormal);
        vec3 u_lightDir_flipped = u_lightDir;
        u_lightDir_flipped.z = -u_lightDir_flipped.z;
        vec3 L = normalize(u_lightDir_flipped);   // from fragment to light

        float NdotL = dot(N, L) > 0 ? 1.0f : 0.0f;

        float shadowFactor = computeShadowFactor(vWorldPos, N);
        float ambientFactor = 0.7f;
        float maxLight = 1.0f;
        float minLight = 0.1f;
        vec3 ambient = ambientFactor * diffuse;
        vec3 lit     = diffuse * max(minLight,(maxLight - ambientFactor) * min(NdotL, shadowFactor)); 
        color   = ambient + lit;
    }
    else
    {
        color = diffuse;
    }
    vec3 finalColor = mix(color, u_fogColor, fog);
    FragColor = vec4(finalColor, 1.0);
}