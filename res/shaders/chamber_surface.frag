#version 330 core

in vec4 vertexColor;
in float v_viewZ;

uniform vec3 u_fogColor;
uniform float u_fogStart;
uniform float u_fogEnd;
out vec4 FragColor;

void main()
{
    //float fogStart = 2500.0;
	//float fogEnd = 25000.0;
    //vec3 fogColor = vec3(0.1, 0.13, 0.2);

	float fog = clamp((v_viewZ - u_fogStart) / (u_fogEnd - u_fogStart), 0.0, 1.0);
    vec3 finalColor = mix( vertexColor.rgb, u_fogColor, fog);

    //FragColor = vertexColor;
	FragColor = vec4(finalColor, 1.0);
}