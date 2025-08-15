#version 330 core
layout(location=0) out uvec4 outData;

flat in uint vSurfaceId;
flat in uint vParentId;
flat in uint vWallSide;

void main() {
    outData = uvec4(vParentId, vSurfaceId, vWallSide, 0u);
}
