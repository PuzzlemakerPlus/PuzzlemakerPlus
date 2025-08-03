#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform float u_size;

void main()
{
    gl_Position = u_proj * u_view * vec4(aPos, 1.0);
    gl_PointSize = u_size; // size in screen space pixels
}