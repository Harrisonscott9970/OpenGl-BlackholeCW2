#version 330 core
// platform.vert  —  Blinn-Phong + PCF Shadow Map receiver
//
// Outputs vLightSpacePos so platform.frag can sample the shadow map.
// The light-space transform (uLightSpaceMatrix) is the same matrix computed
// in SceneBasic_Uniform::renderShadowPass() and bound via setShadowUniforms().

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightSpaceMatrix;   // Week 8: light MVP for shadow coords

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec3 vTangent;
out vec3 vBitangent;
out vec4 vLightSpacePos;          // Week 8: fragment position in light space

void main()
{
    vec4 world    = Model * vec4(aPos, 1.0);
    vWorldPos     = world.xyz;

    mat3 normalMat = mat3(transpose(inverse(Model)));
    vNormal       = normalize(normalMat * aNormal);
    vTangent      = normalize(normalMat * aTangent);
    vBitangent    = normalize(normalMat * aBitangent);

    vUV           = aUV;

    // Shadow coordinate: multiply world position by the light-space matrix.
    // The fragment shader will do the perspective divide and bias itself.
    vLightSpacePos = uLightSpaceMatrix * world;

    gl_Position   = Projection * View * world;
}
