#version 330 core

// asteroid.frag — Cook-Torrance PBR + PCF shadow receive for asteroid belt
//
// Implements the same GGX/Smith/Schlick BRDF as platform.frag but as a
// self-contained shader for the instanced asteroid pass.  Asteroids are
// carbonaceous chondrites: metallic=0, roughness=0.92 (very broad highlight).
// Glow colour and strength vary per-instance: dark when far from the BH,
// hot orange as the asteroid spirals in.

in vec3  vWorldPos;
in vec3  vNormal;
in vec3  vBaseColor;
in vec3  vGlowColor;
in float vGlowStrength;
in vec4  vLightSpacePos;

uniform vec3  uLightPos;
uniform vec3  uCamPos;
uniform vec3  uBHPos;

uniform sampler2D uShadowMap;
uniform int       uShadowEnabled;

layout(location = 0) out vec4 FragColor;

// ── PCF shadow (3x3 kernel for performance on instanced pass) ─────────────────
float calcShadow(vec4 lsPos, vec3 N, vec3 L)
{
    if (uShadowEnabled == 0) return 1.0;
    vec3 proj = lsPos.xyz / max(lsPos.w, 0.0001);
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 ||
        proj.y > 1.0 || proj.z < 0.0 || proj.z > 1.0) return 1.0;

    float cosTheta = max(dot(N, L), 0.0);
    float bias     = max(0.0008 * (1.0 - cosTheta), 0.00012);
    vec2  texel    = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow   = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
        {
            float d = texture(uShadowMap, proj.xy + vec2(x, y) * texel).r;
            shadow += (proj.z - bias > d) ? 1.0 : 0.0;
        }
    return 1.0 - shadow / 9.0;
}

// ── Cook-Torrance BRDF helpers ────────────────────────────────────────────────
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
    // Asteroid material: carbonaceous chondrite — non-metallic, very rough
    const float metallic  = 0.00;
    const float roughness = 0.92;

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos  - vWorldPos);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 H = normalize(L + V);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 F0 = mix(vec3(0.04), vBaseColor, metallic);
    vec3 F  = fresnelSchlick(HdotV, F0);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);

    vec3 specCT     = (D * F * G) / max(4.0 * NdotV * NdotL, 0.001);
    vec3 kD         = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffusePBR = kD * vBaseColor / 3.14159265;

    float shadow = calcShadow(vLightSpacePos, N, L);
    float shade  = mix(0.22, 1.0, shadow);

    vec3 ambient = vBaseColor * 0.04;
    vec3 direct  = (diffusePBR + specCT) * NdotL * shade;

    // Warm BH proximity glow
    float bhDist = length(vWorldPos - uBHPos);
    float bhT    = 1.0 - clamp(bhDist / 1600.0, 0.0, 1.0);
    vec3  bhTint = vec3(0.55, 0.18, 0.02) * bhT * 0.08;

    // Per-instance glow (orange heat as asteroid falls in)
    float rim  = pow(clamp(1.0 - NdotV, 0.0, 1.0), 2.5);
    vec3  glow = vGlowColor * vGlowStrength * (0.06 + rim * 0.25);

    vec3 lit = ambient + direct + bhTint + glow;
    FragColor = vec4(lit, 1.0);
}
