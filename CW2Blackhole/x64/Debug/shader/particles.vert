#version 330 core
// particles.vert — Week 7: Instanced Particle System (dual-type)
//                 Week 9: FBM Noise-driven motion
//
// Type 0 — Accretion particles:
//   Schwarzschild-inspired gravity lerps the particle from spawnPos toward the
//   black hole with a power-curve ease (slow drift → rapid plunge), plus
//   4-octave FBM turbulence that swirls the particle laterally as it falls.
//   Colour & brightness passed to frag as vBrightness (hotter near BH).
//
// Type 1 — Space dust:
//   No infall.  Low-frequency FBM noise drives a gentle volumetric drift
//   through the playfield, creating an ambient haze around the platforms.
//
// Billboarding: camera-right and camera-up are extracted from the view matrix
// rows so the quad always faces the camera regardless of orientation.
// vQuadUV (0..1) is passed for the circular soft-disk alpha in the fragment shader.
//
// References:
//   GPU Gems 3, Ch.23 — instanced particle pattern (per-instance VBO divisor)
//   Perlin, K. (1985). "An image synthesizer." SIGGRAPH '85 — noise basis
//   used in ptFbm() below (value-noise implementation of the Perlin family).

// ── Per-vertex (6-vertex unit quad, location 0) ───────────────────────────────
layout(location = 0) in vec2  aQuadPos;    // -0.5..+0.5, non-instanced

// ── Per-instance (7 floats streamed each frame, locations 1-5) ───────────────
layout(location = 1) in vec3  iSpawnPos;  // world-space spawn position
layout(location = 2) in float iAge;       // 0 = just born, 1 = about to die
layout(location = 3) in float iSize;      // world-unit billboard radius
layout(location = 4) in float iSeed;      // random seed for noise offset
layout(location = 5) in float iType;      // 0 = accretion, 1 = space dust

uniform mat4  uView;
uniform mat4  uProjection;
uniform vec3  uBHPos;
uniform float uTime;
uniform int   uNightVision;   // 0 = normal, 1 = NV mode (dust bright, accretion dim)

out vec2  vQuadUV;       // 0..1 from quad corner — used for radial alpha in frag
out float vAge;
out float vBrightness;
out float vType;
out float vSeed;

// ─────────────────────────────────────────────────────────────────────────────
// Value-noise FBM helpers (Perlin-family, 4 octaves)
// Rotation matrix breaks axis-aligned banding artefacts.
// ─────────────────────────────────────────────────────────────────────────────
float ptHash(vec3 p)
{
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float ptNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);   // C1-continuous smoothstep
    return mix(
        mix(mix(ptHash(i),                ptHash(i + vec3(1,0,0)), f.x),
            mix(ptHash(i + vec3(0,1,0)),  ptHash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(ptHash(i + vec3(0,0,1)),  ptHash(i + vec3(1,0,1)), f.x),
            mix(ptHash(i + vec3(0,1,1)),  ptHash(i + vec3(1,1,1)), f.x), f.y),
        f.z);
}

float ptFbm(vec3 p)
{
    float v = 0.0;
    float a = 0.5;
    mat3 rot = mat3( 0.0,  0.8,  0.6,
                    -0.8,  0.36,-0.48,
                    -0.6, -0.48, 0.64);
    for (int i = 0; i < 4; i++)
    {
        v += a * ptNoise(p);
        p  = rot * p * 2.1;
        a *= 0.5;
    }
    return v;
}

void main()
{
    float ageFade   = 1.0 - smoothstep(0.88, 1.0, iAge);
    float birthFade = smoothstep(0.0, 0.06, iAge);
    float fadeScale = ageFade * birthFade;

    vec3 worldPos;

    if (iType < 0.5)
    {
        // ── Type 0: Accretion particle ────────────────────────────────────────
        // Non-linear ease: slow drift 0-70%, rapid plunge 70-100%
        float gravT   = pow(iAge, 1.6);
        vec3  gravPos = mix(iSpawnPos, uBHPos, gravT);

        // FBM turbulence — time-evolving, co-moving with the accretion plane
        float slowTime = uTime * 0.08;
        vec3  noiseIn  = gravPos * 0.0024 + vec3(iSeed * 0.37, slowTime, iSeed * 0.19);
        float nx = (ptFbm(noiseIn)                         - 0.5) * 2.0;
        float ny = (ptFbm(noiseIn + vec3(5.2, 0.0,  1.3)) - 0.5) * 2.0;
        float nz = (ptFbm(noiseIn + vec3(0.0, 9.7,  2.8)) - 0.5) * 2.0;

        float turbAmt = mix(95.0, 10.0, gravT);   // turbulence fades as gravity wins
        worldPos = gravPos + vec3(nx, ny * 0.25, nz) * turbAmt;

        // Brightness ramps up as particle nears BH (tidal heating).
        // In night-vision mode accretion particles are dimmed so the dust
        // stands out — night vision "tracks dust, not cells/accretion".
        float accrBright = mix(0.55, 2.8, smoothstep(0.55, 0.96, gravT)) * fadeScale;
        vBrightness = (uNightVision == 1) ? accrBright * 0.08 : accrBright;
    }
    else
    {
        // ── Type 1: Space dust — no infall, gentle volumetric drift ──────────
        // Seed coefficients are large so each particle samples the noise from a
        // very different region of the field, even when spawnPos values are close
        // (i.e. inside the same cluster).  This prevents particles from converging
        // to the same world position — each one drifts its own unique path.
        float slowTime = uTime * 0.018;
        vec3  noiseIn  = iSpawnPos * 0.0010
                       + vec3(iSeed * 0.43,
                              slowTime + iSeed * 0.025,
                              iSeed * 0.61);
        float nx = (ptFbm(noiseIn)                         - 0.5) * 2.0;
        float ny = (ptFbm(noiseIn + vec3(3.1, 0.0,  7.4)) - 0.5) * 2.0;
        float nz = (ptFbm(noiseIn + vec3(0.0, 5.6,  1.9)) - 0.5) * 2.0;

        // Drift amplitude 90 units — particles wander within the cloud volume
        // but don't stray so far they blur into neighbouring clouds.
        // Combined with cluster radius 28, net visual cloud radius ~118 units.
        worldPos = iSpawnPos + vec3(nx, ny * 0.58, nz) * 90.0;

        // Normal: clearly visible blue haze blobs.
        // Night-vision: massively amplified — dust becomes the dominant scene
        // element so the player can "fly through" vivid green clouds.
        vBrightness = (uNightVision == 1)
                    ? 4.0 * fadeScale    // very bright → phosphor post-process turns it vivid green
                    : 1.5 * fadeScale;   // normal mode — strong enough to actually see
    }

    // ── Billboard: expand quad in camera-right / camera-up directions ─────────
    vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 camUp    = vec3(uView[0][1], uView[1][1], uView[2][1]);

    float scale = iSize * fadeScale;
    worldPos += (camRight * aQuadPos.x + camUp * aQuadPos.y) * scale;

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);

    // Pass UV (0..1) so the frag shader can compute radial distance from centre
    vQuadUV    = aQuadPos + vec2(0.5);
    vAge       = iAge;
    vType      = iType;
    vSeed      = iSeed;
}
