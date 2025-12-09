#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uint aSurfaceId;
layout(location = 3) in uint aParentId;
layout(location = 4) in uint aWallSide;
layout(location = 5) in vec2 aTexCoord;

uniform mat4 u_view;
uniform mat4 u_proj;

out vec4 vertexColor;
out float v_viewZ;

flat out uint vSurfaceId;
flat out uint vParentId;
flat out uint vWallSide;

out vec3 vWorldPos;  // world position
out vec3 vNormal;    // world normal

void main()
{
    // ----- World space -----
    vWorldPos = aPos;
    vNormal   = normalize(aNormal);

    // ----- Clip-space transform -----
    gl_Position = u_proj * u_view * vec4(aPos, 1.0);

    // Pass view-space depth
    vec4 viewPos = u_view * vec4(aPos, 1.0);
    v_viewZ = -viewPos.z;

    // ----- Your brightness logic -----
    vec3 xAxis = vec3(1,0,0);
    vec3 yAxis = vec3(0,1,0);
    vec3 zAxis = vec3(0,0,1);

    float brightness = 1.0;
    brightness *= 0.674 + 0.326 * (dot(aNormal, zAxis) / 2.0 + 0.5);
    brightness *= 1.0 - 0.131 * abs(dot(aNormal, xAxis));

    vertexColor = vec4(brightness, brightness, brightness, 1.0);

    vSurfaceId = aSurfaceId;
    vParentId  = aParentId;
    vWallSide  = aWallSide;
}
