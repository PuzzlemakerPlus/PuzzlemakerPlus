#version 330 core

out vec2 vUv;

void main()
{
    // Positions for a single oversized fullscreen triangle
    const vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),   // bottom-left
        vec2( 3.0, -1.0),   // far right (off-screen)
        vec2(-1.0,  3.0)    // far top (off-screen)
    );

    vec2 pos = positions[gl_VertexID];

    // Convert clip-space [-1,1] to texture UVs [0,1]
    vUv = pos * 0.5 + 0.5;

    gl_Position = vec4(pos, 0.0, 1.0);
}
