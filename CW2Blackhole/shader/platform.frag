#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vTangent;
in vec3 vBitangent;
in vec4 vLightSpacePos;

uniform vec3  uLightPos;
uniform vec3  uCamPos;
uniform vec3  uBHPos;
uniform float uTime;

uniform vec3  uBaseColor;
uniform vec3  uGlowColor;
uniform float uGlowStrength;

uniform int   uIsCore;
uniform int   uHasDiffuse;
uniform int   uHasNormal;
uniform float uAlpha;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalTex;

uniform vec3  uLightPos2;
uniform vec3  uLightColor2;
uniform float uLightStrength2;

uniform sampler2D uShadowMap;
uniform mat4      uLightSpaceMatrix;
uniform int       uShadowEnabled;

uniform float uMetallic;
uniform float uRoughness;

// Week 9 Noise: 0 = intact platform; >0 = disintegration fraction (max ~0.25)
uniform float uDissolve;

layout(location = 0) out vec4 FragColor;

// ── Week 9: FBM value-noise — surface circuits & disintegration ───────────────
// 4-octave fractional Brownian motion (Perlin-family value noise) drives two
// distinct visual effects:
//   1. Animated energy-circuit etch lines etched into every platform surface.
//   2. Disintegration dissolve on Damaged/Hazard relay platforms: fragments
//      whose noise value falls below uDissolve are discarded, leaving real
//      geometric holes.  The surviving fragments at the crumble boundary glow
//      orange-hot — a "burn edge" simulating superheated metal.
//
// Reference: Perlin, K. (1985). "An image synthesizer." SIGGRAPH '85.
//   The smoothstep C1 interpolant and FBM octave-stacking pattern follow
//   Inigo Quilez's canonical FBM reference: https://iquilezles.org/articles/fbm/
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
    f = f * f * (3.0 - 2.0 * f);          // C1-continuous smoothstep
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
        p  = p * 2.3 + vec2(1.7, 9.2);    // offset each octave to break banding
        a *= 0.5;
    }
    return v;
}

// 0 = normal render, 1 = show shadow map as greyscale for debugging
const int DEBUG_SHADOWS = 0;

// Returns how lit a pixel is (1.0 = fully lit, 0.0 = fully in shadow)
// Shadow calculation — matches the LearnOpenGL Shadow Mapping tutorial
// (learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping) with a 5×5
// PCF kernel instead of the tutorial's 3×3 for softer edges.
float calcShadow(vec4 lightSpacePos, vec3 N, vec3 L)
{
    if (uShadowEnabled == 0) return 1.0;

    // Perspective divide (w == 1 for ortho, but kept for correctness)
    vec3 proj = lightSpacePos.xyz / lightSpacePos.w;
    // Map NDC [-1,1] → texture coords [0,1]
    proj = proj * 0.5 + 0.5;

    // LearnOpenGL: regions outside the light frustum are not in shadow
    if (proj.z > 1.0)
        return 1.0;

    // XY outside the shadow map border → GL_CLAMP_TO_BORDER returns 1.0
    // (far-plane depth), so those samples never contribute shadow.

    float currentDepth = proj.z;

    // Slope-scaled bias (LearnOpenGL formula scaled to our depth range).
    // GL_FRONT culling means back-faces are in the depth map; front faces
    // are always closer to the light (currentDepth < closestDepth), so a
    // tiny bias is sufficient — no self-shadowing risk.
    float cosTheta = max(dot(N, L), 0.0);
    float bias = max(0.0005 * (1.0 - cosTheta), 0.0001);

    // 5×5 PCF — average 25 samples for soft shadow edges
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float closestDepth = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    return 1.0 - shadow;
}

// PBR lighting helper functions
float distributionGGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(3.14159265 * d * d, 0.0001);
}

float geometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 0.0001);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    return geometrySchlickGGX(NdotV, roughness) *
           geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 N = normalize(vNormal);
    if (uHasNormal == 1)
    {
        vec3 T = normalize(vTangent);
        vec3 B = normalize(vBitangent);
        mat3 TBN = mat3(T, B, N);
        vec3 nm = texture(uNormalTex, vUV).rgb * 2.0 - 1.0;
        N = normalize(TBN * nm);
    }

    vec3 V = normalize(uCamPos - vWorldPos);

    vec3 albedo = uBaseColor;
    if (uHasDiffuse == 1)
        albedo *= texture(uDiffuseTex, vUV).rgb;

    vec3 L1 = normalize(uLightPos  - vWorldPos);
    vec3 L2 = normalize(uLightPos2 - vWorldPos);

    float shadowFactor = calcShadow(vLightSpacePos, N, L1);

    if (DEBUG_SHADOWS == 1)
    {
        FragColor = vec4(vec3(shadowFactor), 1.0);
        return;
    }

    // Energy cell / glow sphere rendering
    if (uIsCore == 1)
    {
        // Boost brightness near the centre of the cell
        float radial     = length((vWorldPos - uLightPos2).xz);
        float innerBoost = exp(-radial * 8.0);

        float diff1 = max(dot(N, L1), 0.0);
        vec3  H1    = normalize(L1 + V);
        float spec1 = pow(max(dot(N, H1), 0.0), 48.0);

        float diff2 = max(dot(N, L2), 0.0);
        vec3  H2    = normalize(L2 + V);
        float spec2 = pow(max(dot(N, H2), 0.0), 24.0);

        vec3 ambient  = albedo * 0.08;
        vec3 diffuse  = albedo * diff1 * 0.55 * shadowFactor;
        vec3 specular = vec3(0.40) * spec1 * 0.18 * shadowFactor;
        vec3 fill     = albedo * uLightColor2 * diff2 * (0.12 + uLightStrength2 * 0.35);
        vec3 specFill = uLightColor2 * spec2 * (0.05 + uLightStrength2 * 0.10);

        vec3 lit = ambient + diffuse + specular + fill + specFill;

        float rim = pow(clamp(1.0 - max(dot(N, V), 0.0), 0.0, 1.0), 2.8);

        float pulse = 0.85 + 0.15 * sin(uTime * 3.0);
        lit += uGlowColor * uGlowStrength * pulse;
        lit += uGlowColor * rim * uGlowStrength * 1.2;

        // Animated energy bands scrolling along the cell surface
        float s1 = fract(vUV.y - uTime * 1.10);
        float s2 = fract(vUV.y - uTime * 0.65 + 0.33);
        float s3 = fract(vUV.y - uTime * 1.80 + 0.67);

        float band1 = pow(max(0.0, 1.0 - abs(s1 - 0.5) * 9.0),  3.0);
        float band2 = pow(max(0.0, 1.0 - abs(s2 - 0.5) * 7.0),  3.0) * 0.75;
        float band3 = pow(max(0.0, 1.0 - abs(s3 - 0.5) * 11.0), 3.0) * 0.55;

        float arc1 = 0.55 + 0.45 * sin(vUV.x * 12.566 + uTime *  6.5);
        float arc2 = 0.55 + 0.45 * sin(vUV.x * 25.133 - uTime *  9.1);

        float flicker    = 0.80 + 0.20 * sin(uTime * 17.3) + 0.08 * sin(uTime * 31.7);
        float lineEnergy = (band1 + band2 + band3) * (arc1 * arc2) * flicker;

        // Shift energy line colour toward yellow for a plasma/electric look
        vec3 lineColor = mix(uGlowColor, vec3(1.0, 0.88, 0.15), 0.55);

        lit += lineColor * lineEnergy * uGlowStrength * (2.5 + innerBoost * 5.5);

        float corePulse = 0.75 + 0.25 * sin(uTime * 4.8);
        lit += uGlowColor * uGlowStrength * corePulse * innerBoost * 3.5;
        lit += uGlowColor * rim * uGlowStrength * innerBoost * 2.0;

        FragColor = vec4(lit, uAlpha);
        return;
    }

    // PBR lighting for platforms, debris, and the ship hull
    float NdotV = max(dot(N, V),  0.0);
    float NdotL = max(dot(N, L1), 0.0);
    vec3  H1    = normalize(L1 + V);
    float NdotH = max(dot(N, H1), 0.0);
    float HdotV = max(dot(H1, V), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, uMetallic);
    vec3 F  = fresnelSchlick(HdotV, F0);
    float D = distributionGGX(NdotH, uRoughness);
    float G = geometrySmith(NdotV, NdotL, uRoughness);

    vec3 specCT = (D * F * G) / max(4.0 * NdotV * NdotL, 0.001);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - uMetallic);
    vec3 diffusePBR = kD * albedo / 3.14159265;

    vec3 direct = (diffusePBR + specCT) * NdotL;

    // Secondary fill light
    float diff2 = max(dot(N, L2), 0.0);
    vec3  H2    = normalize(L2 + V);
    float spec2 = pow(max(dot(N, H2), 0.0), 24.0);

    vec3 fill     = albedo * uLightColor2 * diff2 * (0.08 + uLightStrength2 * 0.22);
    vec3 specFill = uLightColor2 * spec2 * (0.03 + uLightStrength2 * 0.08);

    // Reduce lighting in shadow areas (but keep some ambient)
    float shadowDarken = mix(0.22, 1.0, shadowFactor);

    // Higher ambient so faces pointing away from the primary light (e.g. the
    // player-facing side when light comes from the BH direction) remain visible
    // as dark metal rather than completely black.
    vec3 ambient = albedo * 0.10;

    vec3 lit = ambient;
    lit += direct * shadowDarken;
    lit += fill * mix(0.35, 1.0, shadowFactor);
    lit += specFill * mix(0.20, 1.0, shadowFactor);

    // Rim light fades in shadow
    float rim = pow(clamp(1.0 - NdotV, 0.0, 1.0), 2.8);
    lit += uGlowColor * rim * (0.08 + uGlowStrength * 0.16) * mix(0.35, 1.0, shadowFactor);

    // Warm orange tint from the black hole — closer = stronger
    float bhDist = length(vWorldPos - uBHPos);
    float bhT = 1.0 - clamp(bhDist / 1600.0, 0.0, 1.0);
    lit += vec3(0.55, 0.18, 0.02) * bhT * 0.06 * mix(0.55, 1.0, shadowFactor);

    // ── Week 9: Noise — animated circuit etch ────────────────────────────────
    // Two-scale FBM carves thin bright energy-conduit lines into the surface.
    // Coarse scale (×5 UV) sets vein topology; fine scale (×13) adds detail.
    // A 0.01-wide smoothstep band isolates a crisp bright line from the field.
    // Slow uTime drift (×0.025) makes energy appear to flow along conduits.
    float nc      = fbm(vUV * 5.0 + uTime * 0.025);
    float nf      = fbm(vUV * 13.0 + vec2(3.7, 8.1));
    float band    = nc + nf * 0.28;
    float circuit = smoothstep(0.490, 0.500, band) - smoothstep(0.500, 0.510, band);
    lit += uGlowColor * circuit * (0.10 + uGlowStrength * 0.30) * shadowFactor;

    // ── Week 9: Noise — disintegration dissolve ───────────────────────────────
    // Fragments whose noise value falls below uDissolve are discarded outright,
    // cutting physical holes through the platform mesh — visible from both sides.
    // Fragments near the threshold glow orange-hot (burn edge) as tidal forces
    // superheat the crumbling material.  uDissolve is set by the C++ scene:
    //   0.00 = HomeRelay / StableRelay (pristine)
    //   0.13 = DamagedRelay (pitted, partially crumbling)
    //   0.22 = HazardRelay  (severely degraded, near-structural failure)
    if (uDissolve > 0.001)
    {
        float dissolveN = fbm(vUV * 4.5);          // separate static field
        if (dissolveN < uDissolve) discard;

        // Burn edge: within 0.07 above the discard threshold, add incandescent glow
        float burnEdge = smoothstep(uDissolve, uDissolve + 0.07, dissolveN);
        lit += vec3(1.0, 0.28, 0.02) * (1.0 - burnEdge) * 3.0;
    }

    FragColor = vec4(lit, uAlpha);
}