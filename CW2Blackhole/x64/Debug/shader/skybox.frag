#version 330 core
// skybox.frag — Deep space atmosphere matching target aesthetic:
// Dark purple/teal nebula clouds, star field, directional nebula colour wash

in  vec3 vTexCoord;
uniform samplerCube skybox;
uniform float       uTime;

out vec4 FragColor;

// Simple hash for procedural star sparkle
float skHash(float n) { return fract(sin(n) * 43758.5453); }

void main()
{
    vec3 dir = normalize(vTexCoord);
    vec3 col = texture(skybox, dir).rgb;

    // ── Deep space colour grade ───────────────────────────────────────────────
    // Push darks toward deep navy/purple, lift highlights toward teal
    float lum = dot(col, vec3(0.299, 0.587, 0.114));

    // Teal nebula tint in upper hemisphere
    float upFac   = clamp(dir.y * 0.6 + 0.4, 0.0, 1.0);
    vec3 nebTeal  = vec3(0.04, 0.12, 0.22) * upFac;

    // Purple nebula wash on left/right sides
    float sideFac = 1.0 - abs(dir.x) * 0.4;
    vec3 nebPurp  = vec3(0.10, 0.02, 0.18) * sideFac * (1.0 - upFac);

    // Deep void toward BH direction (negative Z in skybox space)
    float voidFac = clamp(-dir.z * 0.6 + 0.3, 0.0, 1.0);
    vec3 voidCol  = vec3(0.02, 0.01, 0.04) * voidFac;

    col = col * 0.65 + nebTeal + nebPurp + voidCol;

    // Colour grade: crush blacks, teal-tint midtones
    col = pow(max(col, vec3(0.0)), vec3(1.15));
    col.b = col.b * 1.12 + 0.005;
    col.g = col.g * 1.06;

    // ── Star twinkle (only bright stars) ─────────────────────────────────────
    float brightness = dot(col, vec3(0.299, 0.587, 0.114));
    if (brightness > 0.55) {
        float flicker = 0.88 + 0.12 * sin(uTime * 4.1 + dir.x * 53.3 + dir.y * 31.7);
        col *= flicker;
    }

    // Subtle warm star clusters toward galactic plane (y ≈ 0)
    float galactic = exp(-dir.y * dir.y * 8.0);
    col += vec3(0.04, 0.02, 0.01) * galactic * lum;

    FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
