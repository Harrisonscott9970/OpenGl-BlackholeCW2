#version 330 core
// shadow_depth.frag  —  Week 8: PCF Shadow Mapping depth pass
//
// Depth-only render pass — the driver writes gl_FragDepth automatically from
// gl_Position.z/gl_Position.w, so this fragment shader does nothing at all.
// It exists because OpenGL 3.3 core requires a fragment shader to be linked.
//
// The resulting depth texture (shadowMap, bound at texture unit 5) is then
// sampled in platform.frag using a 3x3 PCF kernel for soft shadow edges.

void main()
{
    // Intentionally empty.
    // gl_FragDepth is written automatically by the rasteriser from NDC depth.
    // Some drivers require at least a no-op body to avoid undefined behaviour.
}
