#version 330 core
// disk.vert

layout(location = 0) in vec3 aPos;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec3 vWorldPos;
out vec3 vLocal;    // local position (before model transform) for disk UVs

void main()
{
    vLocal      = aPos;
    vec4 world  = Model * vec4(aPos, 1.0);
    vWorldPos   = world.xyz;
    gl_Position = Projection * View * world;
}
