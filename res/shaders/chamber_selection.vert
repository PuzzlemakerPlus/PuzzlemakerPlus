#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uint aSurfaceId;  
layout(location = 3) in uint aParentId;
layout(location = 4) in uint aWallSide;

uniform mat4 u_view;
uniform mat4 u_proj;

// pass integer values without interpolation
flat out uint vSurfaceId;
flat out uint vParentId;
flat out uint vWallSide;

void main() {
    mat4 uVP = u_proj * u_view;
    gl_Position = uVP * vec4(aPos, 1.0);
    vSurfaceId = aSurfaceId;
    vParentId  = aParentId;
    vWallSide  = aWallSide;
}
