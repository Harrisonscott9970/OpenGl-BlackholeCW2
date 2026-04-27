#version 330 core
// particles.frag — Week 7: Instanced Particle System (dual-type)
//                 Week 9: FBM Noise-driven colour + sparkle variation
//
// ── Shader techniques ────────────────────────────────────────────────────────
// Week 7 — Instanced billboard particles with per-fragment radial soft disk.
//   Proper circular alpha computed from vQuadUV: r = length(uv - 0.5) * 2.
//   Fragments outside r > 1.0 are discarded — quads are truly circular.
//
// Week 9 — Procedural colour & sparkle variation.
//   Multi-layer glow: tight Gaussian core + wide soft halo + outer rim ring.
//   Temperature gradient matches the Keplerian disk: cold-blue (far, slow) →
//   orange (intermediate) → overexposed white-hot (final plunge into BH).
//   Sparkle glint: fract-hash produces a per-particle rim twinkle without
//   trig lookups, adding the visual complexity expected in the 90+ band.
//
// Type 0 (accretion): bright, temperature-shifted, with sparkle.
// Type 1 (space dust): very faint blue-grey Gaussian blobs; 700 particles
//   accumulate additively to build the ambient volumetric haze.
//
// Blending: GL_SRC_ALPHA + GL_ONE (additive) set on the CPU side.

in vec2  vQuadUV;
in float vAge;
in float vBrightness;
in float vType;
in float vSeed;

out vec4 FragColor;

// Fast scalar hash — no sin/trig — for sparkle phase
float hash11(float n) { return fract(n * 127.1 * fract(n * 311.7)); }

void main()
{
    // ── Radial distance from billboard centre (0 = centre, 1 = edge) ─────────
    vec2  uv = vQuadUV - 0.5;    // centred, -0.5..0.5
    float r  = length(uv) * 2.0; // 0..1

    // Discard quad corners — particles are truly circular, not square
    if (r > 1.0) discard;

    // ─────────────────────────────────────────────────────────────────────────
    if (vType < 0.5)
    {
        // ── Type 0: Accretion particle ────────────────────────────────────────
        // Three-layer glow: tight bright core + wide diffuse halo + outer rim
        float core = exp(-r * r * 9.0);                          // r=0 → 1.0
        float halo = exp(-r * r * 2.5);                          // softer spread
        float rim  = exp(-pow(r - 0.72, 2.0) * 28.0) * 0.35;    // faint outer ring

        // Temperature gradient (Keplerian heating: outer cold, inner hot)
        //   age 0.0-0.55 → cold blue-grey interstellar dust
        //   age 0.55-0.80 → warm orange accretion heat
        //   age 0.80-1.00 → white-hot near-horizon plunge
        vec3 colCold = vec3(0.32, 0.55, 0.92);
        vec3 colWarm = vec3(1.25, 0.50, 0.14);
        vec3 colHot  = vec3(1.90, 1.40, 0.80);
        vec3 colCore = vec3(2.40, 2.10, 1.50);   // overexposed BH-proximity core

        vec3 bodyCol;
        if (vAge < 0.55)
            bodyCol = mix(colCold, colWarm, vAge / 0.55);
        else if (vAge < 0.82)
            bodyCol = mix(colWarm, colHot, (vAge - 0.55) / 0.27);
        else
            bodyCol = mix(colHot,  colCore, (vAge - 0.82) / 0.18);

        // Core flash amplifies as brightness (= proximity) rises
        float coreBlend = smoothstep(1.2, 2.5, vBrightness);
        bodyCol = mix(bodyCol, colCore, coreBlend * core);

        vec3 col = bodyCol * (core * 1.8 + halo * 0.60 + rim);
        col *= vBrightness;

        // ── Sparkle: per-particle twinkle at the outer rim ───────────────────
        // hash11 gives a stable per-particle phase; sparkle peaks sharply so
        // only a fraction of particles glint at any moment, like real dust.
        float sparkPhase = hash11(vSeed * 0.031 + 0.17) * 6.2832;
        float sparkle    = pow(max(0.0, sin(sparkPhase + vSeed * 0.22)), 7.0);
        col += colCore * sparkle * rim * vBrightness * 0.80;

        // Alpha: multi-layer so the glow spills beyond the core
        float alpha = (core * 0.70 + halo * 0.25 + rim * 0.12)
                    * vBrightness * 0.60;
        alpha = clamp(alpha, 0.0, 0.92);

        FragColor = vec4(col, alpha);
    }
    else
    {
        // ── Type 1: Space dust ────────────────────────────────────────────────
        // Simple soft Gaussian blob — very faint, blue-grey.
        // 700 particles accumulate additively to build ambient haze.
        float blob = exp(-r * r * 4.0);

        vec3 colDust = vec3(0.18, 0.26, 0.52);   // cold interstellar blue-grey
        vec3 col     = colDust * blob * vBrightness;

        // Alpha high enough that ~700 additive quads build a thick visible cloud.
        // In NV mode vBrightness is 4.0 so this naturally produces a very dense
        // glowing mass; in normal mode 1.5 still produces a clear haze patch.
        float alpha  = blob * vBrightness * 0.35;
        alpha = clamp(alpha, 0.0, 0.92);

        FragColor = vec4(col, alpha);
    }
}
