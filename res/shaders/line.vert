#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform int u_screenWidth;
uniform int u_screenHeight;

out vec3 vColor;
out vec2 vUv; // screen space
out float vDepth;

void main() {
    vColor = aColor;
    gl_Position = u_proj * u_view * vec4(aPos, 1.0);
    // calculate screen space coordinate to access depth buffer
    vec3 ndc = gl_Position.xyz / gl_Position.w;
    vUv = ndc.xy * 0.5 + 0.5;
    vDepth = ndc.z * 0.5 + 0.5;
}
