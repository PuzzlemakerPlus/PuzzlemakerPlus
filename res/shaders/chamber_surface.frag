#version 330 core

in vec4 vertexColor;
in float v_viewZ;

flat in uint vSurfaceId;
flat in uint vParentId;
flat in uint vWallSide;

uniform vec3 u_fogColor;
uniform float u_fogStart;
uniform float u_fogEnd;
uniform int u_debugView;

out vec4 FragColor;

void main()
{
    vec3 diffuse;

    float fog = clamp((v_viewZ - u_fogStart) / (u_fogEnd - u_fogStart), 0.0, 1.0);
    if (u_debugView == 1) {
        // Simple visualization: hash IDs to a color
        uint key = vParentId ^ (vSurfaceId << 1) ^ (vWallSide << 5);
        key ^= key >> 16; key *= 0x7feb352du;
        key ^= key >> 15; key *= 0x846ca68bu;
        key ^= key >> 16;
        diffuse = vec3( key        & 255u,
                        (key >> 8)  & 255u,
                        (key >> 16) & 255u) / 255.0;
    }
    else
    {
        diffuse = vertexColor.rgb;
    }
    vec3 finalColor = mix( diffuse, u_fogColor, fog);
	FragColor = vec4(finalColor, 1.0);
}