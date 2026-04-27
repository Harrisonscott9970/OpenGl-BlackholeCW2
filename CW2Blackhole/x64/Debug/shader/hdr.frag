#version 330 core
// hdr.frag — ACES tonemapping + cinematic grade
//
// CW2 Week 5: HDR framebuffer + bloom post-processing
//   • ACES filmic tone mapping
//   • Separable Gaussian bloom (10-pass ping-pong blur in scenebasic_uniform.cpp)
//   • Film grain, scanlines, chromatic aberration (film mode)
//   • Strong vignette to frame the black hole
//
// Grade: deep blacks, orange disk pops, cold space tones

in vec2 vUV;
uniform sampler2D sceneTex;
uniform sampler2D bloomBlurTex;
uniform int       hdrEnabled;
uniform int       bloomEnabled;
uniform int       filmMode;
uniform float     exposure;
uniform float     uTime;
uniform float     uFadeAlpha;      // 0 = no fade, 1 = full black
uniform float     uBHFallDistort;  // 0 = normal, 1 = full BH-fall distortion
uniform float     uDustNearby;     // 0 = clear, 1 = flying through dense dust
uniform int       uNightVision;    // Week 9 Noise: phosphor-green night-vision mode (N key)
uniform float     uBHProximity;    // 0 = safe, 1 = event-horizon approach (pre-cinematic)
out vec4 FragColor;

// ACES filmic tone mapping — fitted curve by Narkowicz, K. (2015).
// "ACES Filmic Tone Mapping Curve." https://knarkowicz.wordpress.com/2016/01/06/
// Constants (a=2.51,b=0.03,c=2.43,d=0.59,e=0.14) reproduce the Academy Color
// Encoding System reference rendering transform (RRT) in a single instruction.
vec3 ACESFilm(vec3 x){
    const float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
}
// h21: 2D → 1D hash function used for film grain and NV shot noise.
// Based on the "canonical" GPU hash; see McGuire, M. (2020).
// "Hash Functions for GPU Rendering." JCGT 9(3). https://jcgt.org/published/0009/03/02/
float h21(vec2 p){
    p=fract(p*vec2(234.34,435.345));p+=dot(p,p+34.23);return fract(p.x*p.y);
}

void main()
{
    // ── BH proximity gravitational distortion (pre-cinematic warning) ────────
    // Radial inward pull + concentric ripple rings, scaled by dangerLevel².
    // Stops before uBHFallDistort takes over for the fall cinematic itself.
    // The pull is strongest at screen-centre (where the BH typically sits)
    // and fades toward the edges so peripheral objects stay readable.
    vec2 sampleUV = vUV;
    if (uBHProximity > 0.0)
    {
        vec2  uvc    = vUV - 0.5;
        float rlen   = length(uvc);
        // Gravitational lens: pull UV toward centre, stronger at mid-ring
        float pull   = uBHProximity * 0.07 * smoothstep(0.65, 0.05, rlen);
        sampleUV    -= uvc * pull;
        // Concentric space-time ripple rings radiating outward
        float ripple = sin(rlen * 24.0 - uTime * 3.2) * uBHProximity * 0.008;
        if (rlen > 0.001) sampleUV += (uvc / rlen) * ripple;
        sampleUV = clamp(sampleUV, 0.001, 0.999);
    }

    // ── Dust proximity: UV distortion + scatter blur ─────────────────────────
    // Runs only in normal mode (NV handles its own look).
    // Step 1 — UV warp: slow sine waves offset the sample coordinate, giving
    //          a refractive shimmer as particles drift across the lens.
    // Step 2 — 9-tap weighted box blur on the warped UV: softens edges and
    //          makes the cloud feel volumetric rather than flat.
    // Both effects scale with uDustNearby so they're imperceptible at low
    // density and peak when the camera is deep inside a cloud.
    if (uDustNearby > 0.04 && uNightVision == 0)
    {
        float dStr = smoothstep(0.04, 0.55, uDustNearby) * 0.010;
        sampleUV += vec2(
            sin(vUV.y * 11.0 + uTime * 0.90) * dStr,
            cos(vUV.x *  9.0 + uTime * 0.65) * dStr
        );
        sampleUV = clamp(sampleUV, 0.001, 0.999);
    }

    vec2 texelSize = 1.0 / vec2(textureSize(sceneTex, 0));
    float blurStr  = smoothstep(0.08, 0.70, uDustNearby);
    vec3 hdr;
    if (blurStr > 0.001 && uNightVision == 0)
    {
        float r = blurStr * 4.0;
        vec3  acc    = vec3(0.0);
        float wTotal = 0.0;
        for (int xx = -1; xx <= 1; ++xx) {
            for (int yy = -1; yy <= 1; ++yy) {
                float w = 1.0 / (1.0 + float(xx*xx + yy*yy));
                acc    += texture(sceneTex, sampleUV + vec2(xx,yy) * texelSize * r).rgb * w;
                wTotal += w;
            }
        }
        hdr = mix(texture(sceneTex, sampleUV).rgb, acc / wTotal, blurStr * 0.75);
    }
    else
    {
        hdr = texture(sceneTex, sampleUV).rgb;
    }

    vec3 bloom = texture(bloomBlurTex, vUV).rgb;

    // Bloom: 0.60 — enough to make the disk and photon ring glow beautifully
    // without washing out the scene (was 0.95 which caused the pastel problem)
    if (bloomEnabled == 1)
        hdr += bloom * 0.58;  // dialled back so disk structure reads clearly

    vec3 mapped = hdr * exposure;

    if (hdrEnabled == 1)
        mapped = ACESFilm(mapped);
    else
        mapped = mapped / (mapped + vec3(1.0));

    mapped = pow(clamp(mapped,0.0,1.0), vec3(1.0/2.2));

    // ── Grade: deep-space cinematic ───────────────────────────────────────────
    float lum = dot(mapped, vec3(0.299,0.587,0.114));

    // Crush shadows to true black — space should be dark, not murky grey
    mapped *= smoothstep(0.0, 0.30, lum);

    // Warm highlight push on bright pixels only (disk, photon ring)
    vec3 hi = clamp(mapped - 0.58, 0.0, 1.0);
    mapped += hi * vec3(0.14, -0.05, -0.14);

    // Gentle S-curve contrast
    mapped = mix(mapped, mapped*mapped*(3.0-2.0*mapped), 0.20);

    if (filmMode == 1) {
        mapped += h21(vUV + fract(uTime*0.017)) * 0.038 - 0.019;
        mapped  = mix(vec3(lum), mapped, 0.88);
        // CRT scanlines
        mapped *= (mod(floor(vUV.y*720.0),3.0)<1.0) ? 0.93 : 1.0;
    }

    // Vignette: frames the BH without killing midtones
    vec2  uv2 = vUV*(1.0-vUV.yx);
    float vig = clamp(pow(uv2.x*uv2.y*14.0, 0.42), 0.0, 1.0);
    mapped *= mix(0.30, 1.0, vig);

    // Chromatic aberration at edges
    float ca = (1.0-vig)*0.004;
    float cr = pow(clamp(ACESFilm(vec3(texture(sceneTex,vUV+vec2(ca,0)).r*exposure)).r,0.0,1.0),1.0/2.2);
    float cb = pow(clamp(ACESFilm(vec3(texture(sceneTex,vUV-vec2(ca,0)).b*exposure)).b,0.0,1.0),1.0/2.2);
    mapped.r = mix(mapped.r, cr, 0.35);
    mapped.b = mix(mapped.b, cb, 0.35);

    // ── BH-fall approach distortion ───────────────────────────────────────────
    // Extreme chromatic aberration + gravitational red-shift as player falls in.
    // uBHFallDistort ramps 0→1 over first 4 s after event-horizon breach.
    if (uBHFallDistort > 0.0)
    {
        // Boost chromatic aberration: stretch red outward, blue inward
        float caBoost = uBHFallDistort * 0.055;
        vec2  uvR = vUV + (vUV - 0.5) * caBoost;
        vec2  uvB = vUV - (vUV - 0.5) * caBoost * 0.7;
        float cr2 = pow(clamp(ACESFilm(vec3(texture(sceneTex, uvR).r * exposure)).r, 0.0, 1.0), 1.0/2.2);
        float cb2 = pow(clamp(ACESFilm(vec3(texture(sceneTex, uvB).b * exposure)).b, 0.0, 1.0), 1.0/2.2);
        mapped.r  = mix(mapped.r, cr2, uBHFallDistort);
        mapped.b  = mix(mapped.b, cb2, uBHFallDistort * 0.6);

        // Red-shift: push all colours toward deep orange
        vec3 redShifted = vec3(mapped.r * 1.45, mapped.r * 0.28, 0.0);
        mapped = mix(mapped, redShifted, uBHFallDistort * 0.75);

        // Tidal stretch vignette: edges darken and pull inward
        vec2 uvc    = vUV - 0.5;
        float tidV  = length(uvc) * uBHFallDistort * 2.8;
        mapped     *= clamp(1.0 - tidV * tidV, 0.0, 1.0);
    }

    // ── Night-vision mode (Week 9: Noise — phosphor-green image intensifier) ───
    // Technique: luminance extraction → power-curve amplification of dark signals
    // → phosphor green remap → electronic noise grain → CRT scan lines.
    // Cuts through space-dust haze because it amplifies dim sources uniformly.
    // Reference: Van Ess, J. (2012). "Real-time Night Vision Rendering".
    //   GPU Pro 3, AK Peters. (intensity amplification + phosphor colour model)
    if (uNightVision == 1)
    {
        // 1. Weighted luminance (ITU-R BT.601 coefficients)
        float lum = dot(mapped, vec3(0.299, 0.587, 0.114));

        // 2. Amplify: gamma < 1 lifts shadows — simulates image-intensifier tube.
        //    Very dark regions (deep space) get a proportional boost so dim
        //    particles and edges become visible without blowing out bright ones.
        float amp = pow(clamp(lum, 0.0, 1.0), 0.55) * 1.30;
        amp = clamp(amp, 0.0, 1.0);

        // 3. Phosphor green remap (P31 phosphor: peak ~530 nm, slight yellow shoulder)
        //    Dark → black, mid → vivid green, bright → yellow-white
        vec3 nvCol = vec3(amp * 0.14,          // red   channel — very dim
                          amp * 1.00,          // green channel — full signal
                          amp * 0.06);         // blue  channel — slight teal tinge

        // 4. Electronic noise grain — simulates photon shot noise in the tube
        float nvGrain = h21(vUV + fract(uTime * 0.091)) * 0.055 - 0.0275;
        nvCol += nvGrain;

        // 5. CRT scan lines (every other pixel row, 4 % dimmer)
        float scanline = (mod(floor(vUV.y * 540.0), 2.0) < 1.0) ? 0.96 : 1.0;
        nvCol *= scanline;

        // 6. Vignette: keep the already-computed vig value from above
        nvCol *= mix(0.50, 1.0, vig);

        // 7. Faint "bloom" on bright objects so hotspots glow on the phosphor
        vec3 nvBloom = texture(bloomBlurTex, vUV).rgb;
        float nvBloomLum = dot(nvBloom, vec3(0.299, 0.587, 0.114));
        nvCol += vec3(0.0, nvBloomLum * 0.35, 0.0);

        mapped = clamp(nvCol, 0.0, 1.0);
    }

    // ── Space-dust haze overlay (normal mode only) ───────────────────────────
    // Adds a cold blue-grey screen-space scatter when surrounded by dust.
    // Skipped in night-vision mode: NV's luminance amplification already makes
    // the bright dust quads dominate the scene, so no extra overlay is needed.
    if (uDustNearby > 0.0 && uNightVision == 0)
    {
        vec3  dustCol    = vec3(0.08, 0.13, 0.32);   // cold interstellar blue
        // Edge weighting: particles scatter most into peripheral vision
        float edgeFactor = 1.0 - smoothstep(0.0, 0.5, vig);
        float hazeMix    = uDustNearby * (0.18 + edgeFactor * 0.22);
        mapped = mix(mapped, dustCol, clamp(hazeMix, 0.0, 0.45));
        // Slight blue tint even at screen centre
        mapped.b += uDustNearby * 0.055;
        mapped = clamp(mapped, 0.0, 1.0);
    }

    // ── Full-screen fade to black ─────────────────────────────────────────────
    mapped = mix(mapped, vec3(0.0), uFadeAlpha);

    FragColor = vec4(clamp(mapped,0.0,1.0),1.0);
}
