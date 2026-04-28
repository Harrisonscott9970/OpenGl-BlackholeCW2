#version 330 core
// shadow_depth.frag  —  Week 8: PCF Shadow Mapping depth pass
//                        Week 9: Disintegration dissolve (matches platform.frag)
//
// Depth-only render pass — the driver writes gl_FragDepth automatically from
// gl_Position.z/gl_Position.w.
//
// Week 9 addition: the same FBM-based discard used in platform.frag is applied
// here so that the shadow map has matching holes — the shadow of a crumbling
// HazardRelay platform has the same gaps as the visible mesh.

in vec2 vUV;
uniform float uDissolve;   // 0 = no dissolve; matches platform.frag uDissolve

// ── Week 9: FBM value-noise (same basis as platform.frag) ────────────────────
float nHash(vec2 p)
{
    p  = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

float nVal(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(nHash(i),                nHash(i + vec2(1.0, 0.0)), f.x),
        mix(nHash(i + vec2(0.0, 1.0)), nHash(i + vec2(1.0, 1.0)), f.x),
        f.y);
}

float fbm(vec2 p)
{
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        v += a * nVal(p);
        p  = p * 2.3 + vec2(1.7, 9.2);
        a *= 0.5;
    }
    return v;
}

void main()
{
    // Match the dissolve pattern used in platform.frag so shadow holes align
    if (uDissolve > 0.001)
    {
        float dissolveN = fbm(vUV * 4.5);
        if (dissolveN < uDissolve) discard;
    }
    // gl_FragDepth written automatically from gl_Position.z/w
}
