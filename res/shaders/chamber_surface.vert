#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Pre-multiplying projection * view matrices prevents access to useful information like z depth
uniform mat4 u_view;
uniform mat4 u_proj;

out vec4 vertexColor;
out float v_viewZ; // Pass Z-depth to fragment shader

void main()
{
	mat4 uVP = u_proj * u_view;
	vec3 xAxis = vec3(1.0f, 0.0f, 0.0f);
	vec3 yAxis = vec3(0.0f, 1.0f, 0.0f);
	vec3 zAxis = vec3(0.0f, 0.0f, 1.0f);

	gl_Position = uVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);

	vec4 viewPos = u_view * vec4(aPos, 1.0);
    v_viewZ = -viewPos.z;

	float brightness = 1.0;
	// Dim as the Z component lowers
	brightness *= 0.674 + 0.326 * (dot(aNormal, zAxis) / 2.0 + 0.5);

	// Lower a bit further for the X axis
	brightness *= 1.0 - 0.131 * abs(dot(aNormal, xAxis));

	vertexColor = vec4(brightness, brightness, brightness, 1.0);
}