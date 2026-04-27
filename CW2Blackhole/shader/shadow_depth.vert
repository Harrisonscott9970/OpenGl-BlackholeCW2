#version 330 core
// shadow_depth.vert  —  Week 8: PCF Shadow Mapping depth pass
//
// This shader runs once per frame from the light's point of view to build
// the shadow map.  Only gl_Position matters here; no colour output needed.
//
// Technique source:
//   Reeves, W.T., Salesin, D.H., Cook, R.L. (1987).
//   "Rendering Antialiased Shadows with Depth Maps."
//   SIGGRAPH Computer Graphics, 21(4), 283-291.
//
// The CPU side sends uLightSpaceMatrix = lightProj * lightView computed in
// SceneBasic_Uniform::renderShadowPass().  Every shadow-casting mesh (platforms,
// asteroids) is drawn through this shader into the depth-only FBO.

layout(location = 0) in vec3 aPos;

uniform mat4 Model;
uniform mat4 uLightSpaceMatrix;

void main()
{
    // Transform directly into light clip-space — no colour varyings needed
    gl_Position = uLightSpaceMatrix * Model * vec4(aPos, 1.0);
}
