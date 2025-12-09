#version 330 core

in vec3 vColor;
in vec2 vUv;
in float vDepth;

out vec4 FragColor;

uniform sampler2D u_depth;
uniform sampler2D u_albedo;

void main() {
    
    float worldDepth = texture(u_depth, vUv).r;
    if(vDepth > worldDepth+0.0001f)
    {
        vec4 worldAlbedo = texture(u_albedo, vUv);
        worldAlbedo.xyz = worldAlbedo.xyz * 0.7f;
        FragColor = worldAlbedo;
    }
    else
    {
        FragColor = vec4(vColor, 1.0);
    }
}
