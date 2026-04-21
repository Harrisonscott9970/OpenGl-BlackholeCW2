#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 View;
uniform mat4 Projection;

out vec3 vTexCoord;

void main()
{
    vTexCoord   = aPos;
    vec4 pos    = Projection * View * vec4(aPos, 1.0);
    gl_Position = pos.xyww;  // force depth = 1.0
}
