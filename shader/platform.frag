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

layout(location = 0) out vec4 FragColor;

// ── Shader techniques implemented ────────────────────────────────────────────
// Week 8 — PCF Shadow Map (5×5 kernel, slope-scaled bias)
//   Reeves, W.T., Salesin, D.H., Cook, R.L. (1987).
//   "Rendering Antialiased Shadows with Depth Maps." SIGGRAPH '87, 283-291.
//
// Week 10 — Physically Based Rendering (Cook-Torrance BRDF)
//   Cook, R.L., Torrance, K.E. (1982). "A Reflectance Model for Computer
//   Graphics." ACM TOG 1(1), 7-24.
//   GGX normal distribution: Walter, B. et al. (2007). "Microfacet Models for
//   Refraction through Rough Surfaces." EGSR 2007.
//   Fresnel: Schlick, C. (1994). "An Inexpensive BRDF Model for Physically-
//   Based Rendering." CGF 13(3), 233-246.

// 0 = normal render
// 1 = show shadow factor as grayscale (useful for demo / debug)
const int DEBUG_SHADOWS = 0;

// ─────────────────────────────────────────────────────────────────────────────
// PCF shadow factor
// returns 1.0 = fully lit, 0.0 = fully shadowed
// ─────────────────────────────────────────────────────────────────────────────
float calcShadow(vec4 lightSpacePos, vec3 N, vec3 L)
{
    if (uShadowEnabled == 0) return 1.0;

    vec3 proj = lightSpacePos.xyz / max(lightSpacePos.w, 0.0001);
    proj = proj * 0.5 + 0.5;

    // Outside shadow map bounds = treat as lit
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z < 0.0 || proj.z > 1.0)
    {
        return 1.0;
    }

    float currentDepth = proj.z;
    float cosTheta = max(dot(N, L), 0.0);

    // Slope-scaled bias (Reeves et al. 1987 / standard PCF practice):
    // steeper-angled surfaces get larger bias to prevent self-shadowing artefacts,
    // but we keep it low enough that the shadow still "sticks" to the caster.
    float bias = max(0.0008 * (1.0 - cosTheta), 0.00012);

    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    // 5x5 PCF gives more stable readable shadowing
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

// ─────────────────────────────────────────────────────────────────────────────
// PBR helpers
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
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

    // ── Emissive objects stay bright ─────────────────────────────────────────
    if (uIsCore == 1)
    {
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

        FragColor = vec4(lit, uAlpha);
        return;
    }

    // ── PBR for platforms / debris / asteroids ───────────────────────────────
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

    // Secondary light
    float diff2 = max(dot(N, L2), 0.0);
    vec3  H2    = normalize(L2 + V);
    float spec2 = pow(max(dot(N, H2), 0.0), 24.0);

    vec3 fill     = albedo * uLightColor2 * diff2 * (0.08 + uLightStrength2 * 0.22);
    vec3 specFill = uLightColor2 * spec2 * (0.03 + uLightStrength2 * 0.08);

    // Stronger readable shadowing:
    // in shadow, keep only a reduced amount of non-ambient lighting
    float shadowDarken = mix(0.22, 1.0, shadowFactor);

    vec3 ambient = albedo * 0.035;

    vec3 lit = ambient;
    lit += direct * shadowDarken;
    lit += fill * mix(0.35, 1.0, shadowFactor);
    lit += specFill * mix(0.20, 1.0, shadowFactor);

    // Rim should not overpower shadow
    float rim = pow(clamp(1.0 - NdotV, 0.0, 1.0), 2.8);
    lit += uGlowColor * rim * (0.08 + uGlowStrength * 0.16) * mix(0.35, 1.0, shadowFactor);

    // BH warmth also reduced inside shadow
    float bhDist = length(vWorldPos - uBHPos);
    float bhT = 1.0 - clamp(bhDist / 1600.0, 0.0, 1.0);
    lit += vec3(0.55, 0.18, 0.02) * bhT * 0.06 * mix(0.55, 1.0, shadowFactor);

    FragColor = vec4(lit, uAlpha);
}