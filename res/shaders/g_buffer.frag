#version 330 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;
in vec3 vertexColor;

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedo;

uniform sampler2D uAlbedoMap; // or a solid color if you don¡¯t use textures yet
uniform int u_useTestChamberColor;
uniform int u_debugView;

vec3 computeTestChamberColor() 
{
    vec3 xAxis = vec3(1,0,0);
    vec3 yAxis = vec3(0,1,0);
    vec3 zAxis = vec3(0,0,1);

    float brightness = 1.0;
    brightness *= 0.674 + 0.326 * (dot(worldNormal, zAxis) / 2.0 + 0.5);
    brightness *= 1.0 - 0.131 * abs(dot(worldNormal, xAxis));

    return vec3(brightness, brightness, brightness);
}

void main()
{
    // normalize just in case
    vec3 N = normalize(worldNormal);
    gNormal = vec4(N, 1.0);

    vec3 albedoColor;
    if(u_debugView == 1)
    {
        albedoColor = vertexColor;
    }   
    else if(u_useTestChamberColor == 1)
    {
		albedoColor = computeTestChamberColor();
	}
    else
    {
        albedoColor = texture(uAlbedoMap, texCoord).rgb; // read from texture
    }
    gAlbedo = vec4(albedoColor, 1.0);

    // depth is written automatically to your depth texture attached to GL_DEPTH_ATTACHMENT
}
