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

// 0 = no minigame, 1 = marker is inside the stability window right now
uniform float uMinigamePulse;

layout(location = 0) out vec4 FragColor;

void main()
{
    // Gaussian falloff across the ribbon width (x goes -1 .. +1)
    float edgeFade = exp(-abs(vLocalUV.x) * 3.2);

    // Bright core line running down the centre
    float core = exp(-abs(vLocalUV.x) * 7.0);

    // Base energy shimmer travelling along the rope
    float shimmer = 0.80 + 0.20 * sin(vLocalUV.y * 14.0 - uTime * 5.5);

    // Minigame stability-window pulse: when the marker is in the hit zone the
    // ribbon amplitude surges — sine wave doubles in speed and the core flares
    float pulseAmp   = uMinigamePulse * (0.60 + 0.40 * sin(uTime * 18.0));
    float pulseCore  = uMinigamePulse * exp(-abs(vLocalUV.x) * 4.0)
                     * (0.50 + 0.50 * sin(vLocalUV.y * 22.0 - uTime * 12.0));

    float totalShimmer = shimmer + pulseAmp;
    float totalCore    = core    + pulseCore * 1.8;

    // Blend glow colour toward bright white during the stability window
    vec3 col = mix(uGlowColor, vec3(1.0, 1.0, 1.0), uMinigamePulse * 0.45)
             * uGlowStrength * totalShimmer * (0.40 + 0.60 * totalCore);

    float alpha = edgeFade * 0.95 * vSegAlpha;

    FragColor = vec4(col, alpha);
}
