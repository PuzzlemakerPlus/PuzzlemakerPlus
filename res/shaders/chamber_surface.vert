#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Pre-multiply projection * view matrices for performance
uniform mat4 uVP;

out vec4 vertexColor;

void main()
{
	gl_Position = uVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);

	float brightness = 1.0;
	// Dim as the Z component lowers
	brightness *= 0.674 + 0.326 * (aNormal.z / 2.0 + 0.5);

	// Lower a bit further for the X axis
	brightness *= 1.0 - 0.131 * abs(aNormal.x);

	vertexColor = vec4(brightness, brightness, brightness, 1.0);
}