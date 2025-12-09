#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uint aSurfaceId;
layout(location = 3) in uint aParentId;
layout(location = 4) in uint aWallSide;
layout(location = 5) in vec2 aTexCoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform int u_debugView;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out vec3 vertexColor;

void main()
{
    if (u_debugView == 1) {
        // ID debug view: no lighting / shadow, just raw color
        uint key = aParentId ^ (aSurfaceId << 1) ^ (aWallSide << 5);
        key ^= key >> 16; key *= 0x7feb352du;
        key ^= key >> 15; key *= 0x846ca68bu;
        key ^= key >> 16;
        vertexColor = vec3( key        & 255u,
                        (key >> 8)  & 255u,
                        (key >> 16) & 255u) / 255.0;
    }
    vec4 world = u_model * vec4(aPos, 1.0);
    worldPos    = world.xyz;
    worldNormal = mat3(transpose(inverse(u_model))) * aNormal;
    texCoord    = aTexCoord;

    gl_Position = u_proj * u_view * world;
}
