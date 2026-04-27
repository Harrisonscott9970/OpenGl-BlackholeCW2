#version 330 core
// hdr.vert — full-screen quad

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main()
{
    vUV         = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
