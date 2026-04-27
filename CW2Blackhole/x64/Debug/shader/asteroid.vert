#version 330 core

// asteroid.vert — per-instance transform for instanced asteroid belt rendering
//
// Week 7 (CW1 carry-over): Instanced rendering.
// Each asteroid supplies its own model matrix and surface colours via a
// per-instance VBO (glVertexAttribDivisor = 1).  A single glDrawElementsInstanced
// call renders all 320 asteroids in one draw call instead of 320 separate calls.

// Per-vertex attributes (divisor = 0)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
// locations 2-4 are uv/tangent/bitangent from the shared sphere VBO (not used here)

// Per-instance attributes (divisor = 1)
layout(location = 5) in vec4  aModel0;       // mat4 column 0
layout(location = 6) in vec4  aModel1;       // mat4 column 1
layout(location = 7) in vec4  aModel2;       // mat4 column 2
layout(location = 8) in vec4  aModel3;       // mat4 column 3
layout(location = 9)  in vec3  aBaseColor;
layout(location = 10) in vec3  aGlowColor;
layout(location = 11) in float aGlowStrength;

uniform mat4 uViewProj;
uniform mat4 uLightSpaceMat;   // for shadow receive

out vec3  vWorldPos;
out vec3  vNormal;
out vec3  vBaseColor;
out vec3  vGlowColor;
out float vGlowStrength;
out vec4  vLightSpacePos;

void main()
{
    mat4 model = mat4(aModel0, aModel1, aModel2, aModel3);

    vec4 worldPos  = model * vec4(aPos, 1.0);
    vWorldPos      = worldPos.xyz;

    // Correct normal transform: transpose(inverse(model))
    mat3 normalMat = transpose(inverse(mat3(model)));
    vNormal        = normalize(normalMat * aNormal);

    vBaseColor     = aBaseColor;
    vGlowColor     = aGlowColor;
    vGlowStrength  = aGlowStrength;
    vLightSpacePos = uLightSpaceMat * worldPos;

    gl_Position = uViewProj * worldPos;
}
