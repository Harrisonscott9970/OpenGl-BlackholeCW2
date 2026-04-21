#version 430 core
// blinnphong.vert
// Standard Blinn-Phong vertex shader.
// NOTE - The full Blinn-Phong implementation used in this project is inside
//       platform.vert / platform.frag (with normal-map and multi-light support).

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vec4 world  = Model * vec4(aPos, 1.0);
    vWorldPos   = world.xyz;
    // Correct normal transform handles non-uniform scaling
    vNormal     = normalize(mat3(transpose(inverse(Model))) * aNormal);
    vUV         = aUV;
    gl_Position = Projection * View * world;
}
