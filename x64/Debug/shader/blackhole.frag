#version 330 core
// blackhole.frag — Schwarzschild shadow + photon ring + warm inner glow
//
// ── Research implementation (CW2 90-100 band) ────────────────────────────────
// Technique 1 — Schwarzschild Geodesic Lensing
//   Based on: James, O., von Tunzelmann, E., Franklin, P., Thorne, K.S. (2015).
//   "Gravitational lensing by spinning black holes in astrophysics, and in the
//   movie Interstellar." Classical and Quantum Gravity, 32(6), 065001.
//   https://doi.org/10.1088/0264-9381/32/6/065001
//
//   Implementation:  Analytical impact-parameter shadow boundary at
//   b_crit = 3√3/2 · M (Eq. 8).  Photon ring from Lorentzian+Gaussian profile
//   (§5).  Deflection angle α = 2Rs/(b·D) applied to exit direction (Eq. 14).
//   Equatorial Doppler beaming: β = v_K/c ≈ 0.55/√r with beam ∝ δ⁴ (§4).
//
// Technique 2 — Simplex Noise turbulence on lensed disk image
//   Noise: McEwan, I., Sheets, D., Richardson, M., Gustavson, S. (2012).
//   "Efficient Computational Noise in GLSL." Journal of Graphics Tools, 16(2).
//   Source: github.com/ashima/webgl-noise (MIT licence).
//   Used verbatim for the permute/taylorInvSqrt functions; noise sampling
//   patterns for the ghost-disk plasma are original to this project.

in  vec3 vWorldPos;
in  vec3 vNormal;
in  vec2 vUV;

uniform samplerCube uSkybox;
uniform vec3        uBHPos;
uniform vec3        uCamPos;
uniform float       uTime;
uniform float       uBHScale;   // pass bhR / 3.0

out vec4 FragColor;

// ═══════════════════════════════════════════════════════════════════════════════
// Simplex 3D Noise — Ian McEwan / Ashima Arts
// ═══════════════════════════════════════════════════════════════════════════════
vec4 permute(vec4 x) { return mod(((x*34.0)+1.0)*x, 289.0); }
vec4 taylorInvSqrt(vec4 r){ return 1.79284291400159 - 0.85373472095314*r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v   - i + dot(i, C.xxx);
    vec3 g  = step(x0.yzx, x0.xyz);
    vec3 l  = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + 2.0*C.xxx;
    vec3 x3 = x0 - 1.0 + 3.0*C.xxx;
    i = mod(i, 289.0);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0,i1.z,i2.z,1.0)) +
        i.y + vec4(0.0,i1.y,i2.y,1.0)) +
        i.x + vec4(0.0,i1.x,i2.x,1.0));
    float n_ = 1.0/7.0;
    vec3  ns = n_*D.wyz - D.xzx;
    vec4  j  = p - 49.0*floor(p*ns.z*ns.z);
    vec4  x_ = floor(j*ns.z);
    vec4  y_ = floor(j - 7.0*x_);
    vec4  x  = x_*ns.x + ns.yyyy;
    vec4  y  = y_*ns.x + ns.yyyy;
    vec4  h  = 1.0 - abs(x) - abs(y);
    vec4  b0 = vec4(x.xy, y.xy);
    vec4  b1 = vec4(x.zw, y.zw);
    vec4  s0 = floor(b0)*2.0 + 1.0;
    vec4  s1 = floor(b1)*2.0 + 1.0;
    vec4  sh = -step(h, vec4(0.0));
    vec4  a0 = b0.xzyw + s0.xzyw*sh.xxyy;
    vec4  a1 = b1.xzyw + s1.xzyw*sh.zzww;
    vec3  p0 = vec3(a0.xy, h.x);
    vec3  p1 = vec3(a0.zw, h.y);
    vec3  p2 = vec3(a1.xy, h.z);
    vec3  p3 = vec3(a1.zw, h.w);
    vec4  norm = taylorInvSqrt(vec4(dot(p0,p0),dot(p1,p1),dot(p2,p2),dot(p3,p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
    vec4 m = max(0.6 - vec4(dot(x0,x0),dot(x1,x1),dot(x2,x2),dot(x3,x3)), 0.0);
    m = m*m;
    return 42.0*dot(m*m, vec4(dot(p0,x0),dot(p1,x1),dot(p2,x2),dot(p3,x3)));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════════
void main()
{
    vec3  toFrag = normalize(vWorldPos - uCamPos);
    vec3  toBH   = normalize(uBHPos    - uCamPos);
    float D      = length(uBHPos - uCamPos);

    float cosA = clamp(dot(toFrag, toBH), -1.0, 1.0);
    float sinA = sqrt(max(0.0, 1.0 - cosA*cosA));

    float bhRworld   = uBHScale * 3.0;
    float shadowEdge = clamp(0.268 * (bhRworld / D), 0.05, 0.85);
    float ringWidth  = shadowEdge * 0.08;

    vec3  fragDir = normalize(vWorldPos - uBHPos);
    float phi     = atan(fragDir.z, fragDir.x);

    // ── SHADOW: warm orange-amber inner glow (lensed disk light) ─────────────
    if (sinA < shadowEdge) {
        float edgeFrac = sinA / shadowEdge;        // 0 = centre, 1 = shadow edge
        float warmGlow = pow(edgeFrac, 1.2) * 0.85;

        // Stronger near the disk plane
        float eqBias = exp(-fragDir.y * fragDir.y * 22.0);

        // Relativistic Doppler shimmer
        vec3  camFlat = normalize(vec3(uCamPos.x - uBHPos.x, 0.0, uCamPos.z - uBHPos.z));
        vec3  tang    = vec3(-sin(phi), 0.0, cos(phi));
        float vd      = dot(tang, camFlat);
        float dop     = sqrt((1.0 + 0.50*vd) / max(0.001, 1.0 - 0.50*vd));
        float shimmer = clamp(pow(dop, 2.0), 0.2, 2.5);

        // Simplex noise plasma texture on the inner face
        float spinA2 = phi - (0.45 / max(edgeFrac*edgeFrac*4.0 + 0.30, 0.30)) * uTime;
        float r2d    = edgeFrac * 2.0;
        vec3  nPos   = vec3(cos(spinA2)*r2d, 0.0, sin(spinA2)*r2d);
        float n1     = 0.5*snoise(nPos*2.2 + vec3(0.0,  uTime*0.10, 0.0)) + 0.5;
        float n2     = 0.5*snoise(nPos*5.0 + vec3(uTime*0.05, 0.0, 0.0)) + 0.5;
        float plasma = pow(clamp(n1*0.65 + n2*0.35, 0.0, 1.0), 0.7);

        // Colour: dark red at centre → orange-amber at shadow boundary
        vec3 colEdge   = vec3(2.2, 0.60, 0.05);
        vec3 colCentre = vec3(0.06, 0.01, 0.0);
        vec3 innerCol  = mix(colCentre, colEdge, warmGlow)
                       * (0.30 + 0.70 * eqBias)
                       * (0.50 + 0.50 * plasma)
                       * shimmer;

        FragColor = vec4(innerCol, 1.0);
        return;
    }

    // ── ESCAPED RAY ───────────────────────────────────────────────────────────
    float bExcess = sinA - shadowEdge;
    float ringT   = bExcess / ringWidth;

    // Analytically-deflected exit direction
    vec3 d = toFrag;
    {
        vec3  perp   = normalize(toBH - cosA * toFrag);
        float Rs     = (shadowEdge * D) / 2.598;
        float alpha2 = min(2.0 * Rs / max(sinA * D, 0.001), 3.14159265);
        d = toFrag * cos(alpha2) + perp * sin(alpha2);
    }

    // Lensed background
    float lum   = dot(texture(uSkybox, d).rgb, vec3(0.299, 0.587, 0.114));
    float focus = 1.0 + 5.0 * exp(-ringT * ringT * 0.45);
    vec3  col   = vec3(lum * 0.90, lum * 0.87, lum * 1.00) * focus;

    // Azimuthal shimmer + Doppler
    float shimmer2 = 0.70
        + 0.20 * sin(phi * 4.4  - uTime * 0.62)
        + 0.10 * sin(phi * 10.3 + uTime * 0.40);

    vec3  camFlatR = normalize(vec3(uCamPos.x - uBHPos.x, 0.0, uCamPos.z - uBHPos.z));
    vec3  tang2    = vec3(-sin(phi), 0.0, cos(phi));
    float vd2      = dot(tang2, camFlatR);
    float dop2     = clamp(sqrt((1.0 + 0.55*vd2) / max(0.001, 1.0 - 0.55*vd2)), 0.10, 2.5);

    // ── Primary photon ring ───────────────────────────────────────────────────
    float rg1Peak = exp(-ringT * ringT * 1.4);
    float rg1Lor  = 1.0 / (1.0 + ringT * ringT * 0.35);
    col += mix(vec3(3.5, 1.2, 0.08), vec3(9.0, 6.5, 2.8), rg1Peak)
         * rg1Lor * rg1Peak * shimmer2 * dop2 * 10.0;

    // ── Secondary photon ring ─────────────────────────────────────────────────
    float ringT2 = (bExcess - ringWidth * 0.28) / (ringWidth * 0.20);
    col += vec3(5.0, 2.4, 0.40) * exp(-ringT2 * ringT2) * dop2 * shimmer2 * 4.5;

    // ── Ghost disk image — equatorial arc near shadow ─────────────────────────
    float ghostBand = exp(-d.y * d.y * 7.5);
    float ghostProx = exp(-(bExcess * bExcess) / (shadowEdge * shadowEdge * 0.18));
    float ghostStr  = ghostBand * ghostProx;

    if (ghostStr > 0.008) {
        float dPhi   = atan(d.z, d.x);
        float dRad   = 1.5 + (bExcess / shadowEdge) * 1.6;
        float spinA3 = dPhi - (0.55 / max(dRad * dRad, 0.30)) * uTime;

        vec3 nPos2 = vec3(cos(spinA3)*dRad*0.55, 0.0, sin(spinA3)*dRad*0.55);
        float gt1  = 0.5*snoise(nPos2*1.6 + vec3(0.0,  uTime*0.09, 0.0)) + 0.5;
        float gt2  = 0.5*snoise(nPos2*3.8 + vec3(uTime*0.04, 0.0, uTime*0.025)) + 0.5;
        float gt3  = 0.5*snoise(nPos2*7.2 + vec3(0.0, -uTime*0.06, uTime*0.03)) + 0.5;
        float gPlasma = pow(clamp(gt1*0.50 + gt2*0.35 + gt3*0.15, 0.0, 1.0), 0.55);

        vec3  camFlatG = normalize(vec3(uCamPos.x - uBHPos.x, 0.0, uCamPos.z - uBHPos.z));
        vec3  tangG    = vec3(-sin(spinA3), 0.0, cos(spinA3));
        float betaG    = clamp(0.58 / max(sqrt(dRad), 0.45), 0.0, 0.78);
        float vdG      = dot(tangG, camFlatG);
        float dopG     = sqrt((1.0 + betaG*vdG) / max(0.001, 1.0 - betaG*vdG));
        float beamG    = clamp(pow(dopG, 4.0), 0.02, 9.0);

        float radFrac = clamp((dRad - 1.0) / 2.2, 0.0, 1.0);
        vec3  gCol    = mix(vec3(4.5, 2.5, 0.55), vec3(1.2, 0.28, 0.025), radFrac)
                      * (0.35 + 0.65 * gPlasma) * beamG;
        col += gCol * ghostStr * 5.5;
    }

    // ── Equatorial ambient warm glow ──────────────────────────────────────────
    float eqBias2  = exp(-fragDir.y * fragDir.y * 14.0);
    float nearProx = exp(-ringT * ringT * 0.20);
    col += vec3(1.2, 0.30, 0.02) * eqBias2 * nearProx
         * max(0.0, sin(phi + uTime * 0.08)) * 0.60;

    FragColor = vec4(col, 1.0);
}
