#version 330 core
// blackhole.vert — passes world-space position and UV through for ray-marched lensing

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vec4 worldPos = Model * vec4(aPos, 1.0);
    vWorldPos     = worldPos.xyz;
    vNormal       = normalize(mat3(transpose(inverse(Model))) * aNormal);
    vUV           = aUV;
    gl_Position   = Projection * View * worldPos;
}
