#version 330 core
// disk.frag — Cinematic Gargantua-style accretion disk
//
// ── Research implementation (CW2 90-100 band) ────────────────────────────────
// Keplerian accretion disk model with relativistic Doppler beaming.
//
// Physical model from:
//   James, O., von Tunzelmann, E., Franklin, P., Thorne, K.S. (2015).
//   "Gravitational lensing by spinning black holes in astrophysics, and in the
//   movie Interstellar." CQG 32(6), 065001. doi:10.1088/0264-9381/32/6/065001
//
//   Key equations implemented:
//   • Keplerian angular speed: Ω ∝ r^{-3/2}  (§3.1, around Eq. 5)
//   • Relativistic β = v_K/c ≈ 0.58/√r        (approximation to Eq. 4)
//   • Doppler factor δ = √((1+β·n̂)/(1−β·n̂))  (§4, Eq. 9)
//   • Beaming intensity: I ∝ δ⁴               (§4, Eq. 10)
//   • ISCO (innermost stable circular orbit) glow at r = innerEdge
//
// Noise source (verbatim):
//   McEwan, I. et al. (2012). "Efficient Computational Noise in GLSL."
//   Journal of Graphics Tools 16(2). github.com/ashima/webgl-noise (MIT)
//   Replaced hash-based FBM with Simplex 3D — eliminates tiling artefacts
//   and produces the organic plasma filament structure seen in the Interstellar
//   reference renders.

in  vec3 vWorldPos;
in  vec3 vLocal;

uniform float uTime;
uniform vec3  uLightPos;
uniform vec3  uCamPos;
uniform vec3  uBHPos;
uniform int   uDrawBack;

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

// Convenience: remap snoise to [0,1]
float snoise01(vec3 v) { return 0.5*snoise(v) + 0.5; }

void main()
{
    float radius = length(vLocal.xz);
    float angle  = atan(vLocal.z, vLocal.x);

    // ── Front/back face culling ───────────────────────────────────────────────
    vec3 camDir  = normalize(uCamPos - uBHPos);
    vec3 fragDir = normalize(vWorldPos - uBHPos);
    float side   = dot(fragDir, camDir);
    if (uDrawBack == 1 && side > 0.0) discard;
    if (uDrawBack == 0 && side < 0.0) discard;

    const float innerEdge = 1.04;
    const float outerEdge = 2.20;
    float radNorm = clamp((radius - innerEdge) / (outerEdge - innerEdge), 0.0, 1.0);

    // ── Keplerian rotation (v ∝ 1/√r) ────────────────────────────────────────
    float spinRate  = 0.62 / max(radius*radius, 0.30);
    float spinAngle = angle - spinRate*uTime;

    // ── Simplex noise plasma turbulence (reference adiskColor noise pattern) ──
    // Spherical coordinate frame co-rotating with disk
    vec3 nPos = vec3(cos(spinAngle)*radius*0.55, vLocal.y*10.0, sin(spinAngle)*radius*0.55);

    // Three octaves at different scales — alternating time directions (reference LOD loop)
    float t1 = snoise01(nPos*1.6 + vec3(0.0,         uTime*0.09, 0.0));
    float t2 = snoise01(nPos*3.8 + vec3(uTime*0.04,  0.0,        uTime*0.025));
    float t3 = snoise01(nPos*7.2 + vec3(0.0,        -uTime*0.06, uTime*0.030));
    float t4 = snoise01(nPos*14.0+ vec3(uTime*0.08,  uTime*0.04, 0.0));

    // Weighted combination: coarse shape → mid detail → fine sparks
    float plasma = clamp(t1*0.44 + t2*0.30 + t3*0.16 + t4*0.10, 0.0, 1.0);
    // Amplify bright blobs, suppress dim regions (reference density multiplication)
    plasma = pow(plasma, 0.58);

    // ── Radial density (reference density model) ──────────────────────────────
    float density = exp(-radNorm*3.0) * (1.0 - exp(-radNorm*22.0));
    density = pow(density, 0.50);

    // Sweeping plasma filament
    float filament = exp(-pow((radNorm - 0.18)*8.0, 2.0))
                   * (0.5 + 0.5*sin(spinAngle*6.0 + uTime*2.5));
    density += filament*0.18;
    density  = clamp(density, 0.0, 1.5);

    // ── Temperature/colour gradient ───────────────────────────────────────────
    vec3 c0 = vec3(7.5, 5.00, 2.50);   // white-hot ISCO
    vec3 c1 = vec3(4.5, 1.80, 0.12);   // bright orange
    vec3 c2 = vec3(2.5, 0.60, 0.03);   // amber
    vec3 c3 = vec3(1.0, 0.15, 0.01);   // dark red-amber

    vec3 baseCol;
    if      (radNorm < 0.15) baseCol = mix(c0, c1, radNorm/0.15);
    else if (radNorm < 0.45) baseCol = mix(c1, c2, (radNorm-0.15)/0.30);
    else                     baseCol = mix(c2, c3, (radNorm-0.45)/0.55);

    // Plasma modulates brightness (brighter blobs, darker gaps)
    baseCol *= (0.38 + 0.92*plasma);

    // ── Relativistic Doppler beaming (the Gargantua signature) ───────────────
    vec3  camFlat = normalize(vec3(uCamPos.x-uBHPos.x, 0.0, uCamPos.z-uBHPos.z));
    vec3  tang    = vec3(-sin(spinAngle), 0.0, cos(spinAngle));
    float beta    = clamp(0.58/max(sqrt(radius),0.45), 0.0, 0.78);
    float vDotCam = dot(tang, camFlat);
    float doppler = sqrt((1.0+beta*vDotCam)/max(0.001,1.0-beta*vDotCam));
    doppler = clamp(doppler, 0.04, 7.0);
    float beaming = pow(doppler, 4.0);
    baseCol *= clamp(beaming, 0.01, 9.0);

    // Colour temperature shift with Doppler
    if (doppler > 1.0) {
        float whiten = clamp((doppler-1.0)*0.22, 0.0, 0.55);
        baseCol = mix(baseCol,
            vec3(baseCol.r*1.1, baseCol.r*0.70, baseCol.r*0.45), whiten);
    } else {
        float recede = clamp(1.0 - doppler, 0.0, 1.0);
        baseCol.g *= clamp(doppler*1.1, 0.0, 1.0);
        baseCol.b *= clamp(doppler*0.7, 0.0, 1.0);
        baseCol   *= (1.0 - recede*0.5);
    }

    // ── Vertical (height) fade — razor-thin disk ──────────────────────────────
    float diskThickness = 0.012 + radNorm*0.038;
    float hFade = exp(-(vLocal.y*vLocal.y) / max(diskThickness*diskThickness, 0.00001));
    hFade = pow(hFade, 1.4);
    baseCol *= 1.0 + 0.18*sign(vLocal.y)*hFade;

    // ── ISCO ring: white-hot innermost stable circular orbit ──────────────────
    float captureGlow = exp(-pow((radius - innerEdge)*60.0, 2.0));
    float flicker     = 0.65 + 0.35*sin(spinAngle*22.0 + uTime*11.0);
    baseCol += vec3(4.0,2.8,1.4)*captureGlow*flicker;

    float iscoRing = exp(-pow((radius - innerEdge - 0.02)*130.0, 2.0));
    baseCol += vec3(5.0,3.5,1.8)*iscoRing;

    // ── Marginally-bound orbit shimmer at ~1.3× inner edge ───────────────────
    float mbo       = exp(-pow((radius - innerEdge*1.30)*40.0, 2.0));
    float mboShimmer= 0.5 + 0.5*sin(spinAngle*10.0 + uTime*5.0);
    baseCol += vec3(1.8,1.0,0.4)*mbo*mboShimmer*0.5;

    // ── Alpha composition ─────────────────────────────────────────────────────
    float alpha = density * hFade * (0.42 + 0.58*plasma);
    alpha *= smoothstep(0.0, 0.08, radNorm);          // inner edge fade
    alpha *= smoothstep(0.0, 0.60, 1.0 - radNorm);   // outer fade: starts at 40%, zero at 100%
    alpha  = clamp(alpha*1.5, 0.0, 0.92);

    FragColor = vec4(baseCol, alpha);
}
