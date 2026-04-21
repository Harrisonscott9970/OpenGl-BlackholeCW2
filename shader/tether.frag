#version 330 core

// tether.frag  —  Glowing tether-rope ribbon fragment shader
//
// Receives billboard UV coordinates from the geometry shader and produces
// a soft-edged luminous rope with an animated energy shimmer running along it.
// Alpha blending (GL_SRC_ALPHA, GL_ONE) in the caller produces an additive
// glow that is brighter at the centre and fades to nothing at the edges.

in vec2  vLocalUV;   // x = -1..1 across ribbon, y = 0..1 along segment
in float vSegAlpha;  // reserved (currently 1.0)

uniform vec3  uGlowColor;
uniform float uGlowStrength;
uniform float uTime;

layout(location = 0) out vec4 FragColor;

void main()
{
    // Gaussian falloff across the ribbon width (x goes -1 .. +1)
    float edgeFade = exp(-abs(vLocalUV.x) * 3.2);

    // Bright core line running down the centre
    float core = exp(-abs(vLocalUV.x) * 7.0);

    // Animated energy pulse travelling along the rope (v = 0..1 per segment)
    float shimmer = 0.80 + 0.20 * sin(vLocalUV.y * 14.0 - uTime * 5.5);

    vec3  col   = uGlowColor * uGlowStrength * shimmer * (0.40 + 0.60 * core);
    float alpha = edgeFade * 0.95 * vSegAlpha;

    FragColor = vec4(col, alpha);
}
