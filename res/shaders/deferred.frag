#version 330 core

in vec2 vUv;

out vec4 FragColor;

uniform mat4 u_view;
uniform mat4 u_invView;
uniform mat4 u_viewToLight;
uniform mat4 u_proj;
uniform mat4 u_invProj;
uniform vec2 u_screenSize;
uniform float u_near;
uniform float u_far;

uniform mat4 u_lightVP;
uniform vec3 u_lightDir;
uniform int u_drawShadow;

uniform int u_debugView;

uniform vec3 u_fogColor;
uniform float u_fogStart;
uniform float u_fogEnd;

uniform sampler2D u_shadowMap;
uniform sampler2D u_normal;
uniform sampler2D u_albedo;
uniform sampler2D u_depth;
uniform sampler2D u_ao;

float viewZFromDepth(float depth)
{
    // depth: [0,1] from depth buffer
    float z_ndc = depth * 2.0 - 1.0; // back to NDC [-1,1]

    // Linearize depth: returns positive distance from camera
    float z_view = (2.0 * u_near * u_far) /
                   (u_far + u_near - z_ndc * (u_far - u_near));

    // This z_view == -viewSpace.z (your old -viewPos.z)
    return z_view;
}
vec3 getViewPos(vec2 uv, float depth)
{
    float z = depth * 2.0 - 1.0;          // NDC
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = u_invProj * clip;
    return view.xyz / view.w;
}
float linearizeDepth(float depth, float nearPlane, float farPlane)
{
    float z = depth * 2.0 - 1.0; // back to NDC (-1..1)
    return (2.0 * nearPlane * farPlane) /
           (farPlane + nearPlane - z * (farPlane - nearPlane));
}
// --------- Shadow helper ---------
float computeShadowFactorView(vec3 viewPos, vec3 normal)
{
    // Transform to light clip space
    vec4 lightSpace = u_viewToLight * vec4(viewPos, 1.0);
    vec3 projCoords = lightSpace.xyz / lightSpace.w;
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
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(u_shadowMap, projCoords.xy + offset).r;
            shadow += ((currentDepth - bias) > (closestDepth)) ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}
void main()
{
    // Fetch G-buffer data
    vec3 albedo = texture(u_albedo, vUv).rgb;
    //this is how you switch from view space to world space if necessary:
        //vec3 N_view      = texture(u_normal, vUv).xyz;
        //vec3 N = normalize(mat3(u_invView) * N_view);
    vec3 N = texture(u_normal, vUv).xyz;
    float depth = texture(u_depth, vUv).r;
    vec3 ao = texture(u_ao, vUv).rgb;
    // If nothing was drawn here, depth is usually 1.0 (far plane)
    if (depth == 1.0)
    {
        // Puzzlemaker background color
        FragColor = vec4( 0.803, 0.815, 0.807, 1.0);
        return;
    }
    N = normalize(N);
    
    vec3 color;
    if(u_drawShadow == 1)
    {
//      ----- Lighting + shadow -----
        vec3 viewPos = getViewPos(vUv, depth);
        vec3 u_lightDir_flipped = u_lightDir;
        u_lightDir_flipped.z = -u_lightDir_flipped.z;
        vec3 L = normalize(u_lightDir_flipped);  // direction from fragment to light

        float NdotL = max(dot(N, L), 0.0);       // smooth Lambert, no hard step

        float shadowFactor = clamp(computeShadowFactorView(viewPos, N), 0.0, 1.0);

        float ambientFactor = 0.7;
        float minLight = 0.1;
        float maxLight = 1.0;

        // how much direct light we actually get
        float lightTerm = NdotL * shadowFactor;

        // interpolate between minLight and maxLight based on lightTerm
        float directScale = minLight + (maxLight - minLight) * lightTerm;

        vec3 ambient = ambientFactor * albedo;
        vec3 lit     = albedo * directScale;

        color = clamp(ambient + lit, 0,1);
    }
    else
    {
        color = albedo;
    }
    float v_viewZ = viewZFromDepth(depth);
    float fog = clamp((v_viewZ - u_fogStart) / (u_fogEnd - u_fogStart), 0.0, 1.0);
    vec3 finalColor = mix(color, u_fogColor, fog) * ao; // * ao;
    FragColor = vec4(finalColor, 1.0);
    
}