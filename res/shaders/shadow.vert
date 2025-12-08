#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 u_lightVP;

void main()
{
    gl_Position = u_lightVP * vec4(aPos, 1.0);
}
