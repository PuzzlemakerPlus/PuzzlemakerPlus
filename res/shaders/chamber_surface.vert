#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Pre-multiply projection * view matrices for performance
uniform mat4 uVP;

out vec4 vertexColor;

void main()
{
	vec3 xAxis = vec3(1.0f, 0.0f, 0.0f);
	vec3 yAxis = vec3(0.0f, 1.0f, 0.0f);
	vec3 zAxis = vec3(0.0f, 0.0f, 1.0f);

	gl_Position = uVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);

	float brightness = 1.0;
	// Dim as the Z component lowers
	brightness *= 0.674 + 0.326 * (dot(aNormal, zAxis) / 2.0 + 0.5);

	// Lower a bit further for the X axis
	brightness *= 1.0 - 0.131 * abs(dot(aNormal, xAxis));

	vertexColor = vec4(brightness, brightness, brightness, 1.0);
}