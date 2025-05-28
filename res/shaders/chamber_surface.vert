#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec4 vertexColor;

void main()
{
   gl_Position = projection * view * vec4(aPos.x, aPos.y, aPos.z, 1.0);
   vertexColor = vec4(aPos.x / 128.0, aPos.y / 128.0, aPos.z / 128.0, 1.0);
}