# TON 618: Stellar Expedition — COMP3015 CW2

**Student:** Harrison Scott  
**Student ID:** 10805603  
**Module:** COMP3015 – Games Graphics Pipelines  
**Module Leader:** Dr Ji-Jian Chin  
**Repository:** https://github.com/Harrisonscott9970/OpenGl-BlackholeCW2  
**Video Demo:** [insert YouTube URL]

---

## Project Overview

TON 618: Stellar Expedition is a real-time OpenGL / GLSL graphics project created for COMP3015 CW2.

The project develops my original CW1 black hole prototype into a broader and more technically ambitious interactive scene built around multiple advanced rendering techniques from Weeks 5–10 of the module. The player pilots a spacecraft near TON 618, exits in EVA mode to recover energy cells from relay platforms, and must return safely before being pulled into the black hole.

The focus of the coursework is not simply the inclusion of several rendering effects, but the integration of those effects into one coherent scene and gameplay loop. The project combines:

- HDR tonemapping and bloom
- shadow mapping with PCF filtering
- instanced particles
- noise-driven procedural environmental motion and variation
- Cook-Torrance-style metallic/roughness shading
- a geometry-shader tether ribbon
- a research-inspired black hole visualisation effect

These systems are tied together with HUD overlays, menu presentation, a tether recovery minigame, score persistence, and cinematic fail / win presentation so that the result functions as a playable and polished graphics project rather than a disconnected collection of technical demonstrations.

---

## Visual Overview

The screenshots below show the project's key presentation and gameplay systems in context.

### View of CW1 scene
<img width="794" height="546" alt="image" src="https://github.com/user-attachments/assets/10752ab8-1b9c-4bf7-896b-8054cdb71a56" />

### Main Gameplay View of CW2
[Main gameplay view - black hole, accretion disk, cockpit HUD, and environmental scene]<img width="799" height="605" alt="image" src="https://github.com/user-attachments/assets/deb86fb5-7042-4412-8491-53801d5fc5c0" />

### EVA Tether Recovery Minigame
[Tether recovery minigame showing stabilisation UI and gameplay feedback]<img width="801" height="605" alt="image" src="https://github.com/user-attachments/assets/98be1810-b262-4ef5-be5c-f88e8ce05204" />

### Night Vision Post-Process Mode
[Night vision mode showing alternate post-processing pipeline during gameplay]<img width="807" height="601" alt="image" src="https://github.com/user-attachments/assets/45913f2d-a412-4ba2-bdaa-97187de1520e" />

### Main Menu Presentation
[Animated main menu presentation screen]<img width="802" height="603" alt="BHMM" src="https://github.com/user-attachments/assets/a78a594d-f2ce-4350-bd9f-a1760a727bcd" />

---

## Development Progress from CW1 to CW2

Although this project is built on the visual and thematic foundation established in CW1, the CW2 submission is a substantial extension rather than a simple resubmission.

The original CW1 project focused on a stylised black hole scene with post-processing, shader-based presentation, and a simple collect-and-return gameplay loop. For CW2, that prototype was expanded into a more technically ambitious and more complete interactive system.

| Area | CW1 | CW2 Extension |
|---|---|---|
| Core Visual Theme | Stylised black hole scene | Expanded black hole scene with broader rendering pipeline and stronger presentation systems |
| Post-Processing | HDR / bloom | HDR / bloom plus additional post-process control and night-vision mode |
| Gameplay | Collect energy cells and return | EVA mode, tether danger, recovery minigame, score persistence, leaderboard, cinematic fail/win flow |
| Rendering Features | Earlier prototype-level feature stack | Shadows with PCF, instanced particles, geometry shader tether ribbon, noise-driven environmental motion, Cook-Torrance-style PBR |
| Scene Structure | Smaller prototype structure | More modular multi-file scene organisation with clearer per-system responsibilities |
| Presentation | Interactive prototype | Menu systems, loading screen, HUD overlays, visual feedback states, stronger game-style framing |

The aim of CW2 was therefore not just to increase the number of features, but to build a more coherent real-time graphics project in which rendering, presentation, and gameplay systems support one another.

---

## Build Information

| Item | Detail |
|---|---|
| IDE | Visual Studio 2022 |
| OpenGL Version | 4.3 Core Profile |
| Template Base | COMP3015 Lab 1 template |
| Libraries | GLAD, GLFW, GLM, stb_image, irrKlang |

---

## Running the Project

You can run the project in either of these ways:

- Open `Project_Template.sln` in Visual Studio and run with **Local Windows Debugger**
- Or run the built executable from the included build folder in the submission archive

The submission zip contains:

- the full Visual Studio solution
- the executable build
- all required assets and shader files
- this README
- the signed Generative AI declaration
- AI prompt / transcript evidence for claimed features

---

## Controls

| Key / Input | Action |
|---|---|
| W / A / S / D | Move |
| Mouse | Look / aim |
| X | Toggle EVA / cockpit mode |
| Left Mouse | Collect nearby energy cell in EVA mode |
| Shift | Boost |
| A / D | Tether minigame input |
| N | Toggle night-vision mode |
| H | Toggle HDR |
| B | Toggle bloom |
| F | Toggle film mode |
| Q / E | Exposure down / up |
| G | Reset mission |
| Escape | Quit |

---

## Gameplay Summary

The main gameplay loop is:

1. Start from the home platform
2. Fly to relay platforms positioned around the black hole
3. Leave the ship in EVA mode
4. Collect all energy cells
5. Return to the home platform to complete the mission

The black hole creates increasing danger as the player moves closer. In EVA mode, drifting too far from the ship triggers a tether recovery minigame. Moving too close to the black hole triggers a cinematic fail sequence before the mission resets.

Scores are written to `scores.txt`, and the menu includes a leaderboard showing the top recorded runs.

This gameplay structure was designed so that the project would function as a genuine interactive scene with goals, challenge, failure, recovery, and persistence rather than only as a visual demonstration.

---

## Feature Coverage

## Week 5 — HDR Tonemapping and Bloom

**Files:** `shader/hdr.frag`, `shader/blur.frag`, `scenebasic_uniform.cpp`

The main scene is first rendered into a floating-point HDR framebuffer. A separable Gaussian blur is then applied using a ping-pong framebuffer setup to produce bloom before the final composite pass.

The HDR composite shader supports:

- exposure adjustment
- bloom toggle
- film-style post-processing
- vignette
- grain
- scanline effects
- black-hole proximity distortion
- fail-sequence visual distortion

This gives the project a stronger cinematic look and allows bright effects such as the accretion disk, particles, pickups, and emissive scene elements to glow more convincingly.

---

## Week 6 — Geometry Shader Tether Ribbon

**Files:** `shader/tether.vert`, `shader/tether.geom`, `shader/tether.frag`, `SceneTether.cpp`

The EVA tether is simulated on the CPU as a segmented rope and rendered with a geometry shader that expands the line into a camera-facing ribbon.

The tether changes colour and apparent intensity as strain increases, giving the player clear visual feedback when they are approaching the maximum tether distance. This helps connect the geometry-shader rendering feature directly to gameplay rather than using it as an isolated technical demonstration.

---

## Week 7 — Instanced Particle System

**Files:** `shader/particles.vert`, `shader/particles.frag`, `SceneParticles.cpp`

The particle system is rendered using instancing so that a large number of billboard particles can be drawn efficiently in a small number of draw calls.

Two main particle roles are used:

- particles around the black hole / accretion region
- drifting space-dust particles throughout the scene

Particle motion is influenced by lifetime, black hole pull, and noise-based turbulence so that the environment feels more dynamic and less empty. This feature helped improve both scene atmosphere and rendering scale.

---

## Week 8 — Shadow Mapping with PCF

**Files:** `shader/shadow_depth.vert`, `shader/shadow_depth.frag`, `shader/platform.frag`, `scenebasic_uniform.cpp`

A shadow map is rendered from the light's point of view before the main scene pass.

In the main platform/material shader, Percentage Closer Filtering (PCF) is used to soften shadow edges and reduce aliasing. A depth bias is also applied to reduce acne and self-shadowing artefacts.

This is one of the main advanced rendering features used to satisfy the CW2 requirement, and it improves depth, grounding, and readability in the platform areas of the scene.

---

## Week 9 — Noise / Procedural Environmental Motion

**Files:** `shader/particles.vert`, `shader/blackhole.frag`, `shader/disk.frag`, `scenebasic_uniform.cpp`

Noise-based logic is used as a dedicated CW2 feature to add procedural variation and environmental motion across the scene.

This is not treated only as a small decorative effect. Instead, noise and procedural motion are used to make the space environment feel more alive, less repetitive, and less manually scripted.

Examples include:

- turbulence and drift variation in the particle system
- animated distortion and instability around the black hole visuals
- subtle animated variation in the accretion disk surface
- procedural environmental motion in surrounding debris and scene movement behaviour

The procedural asteroid / environmental motion work was intentionally carried forward into CW2 as part of the noise-based feature set rather than being fully claimed in CW1. In this submission, that procedural variation is treated as part of the project's broader environmental design and scene animation strategy.

Using noise across several interconnected systems also helped make the world feel more cohesive rather than limiting procedural behaviour to one isolated effect.

---

## Additional Post-Process Mode — Night Vision

**Files:** `shader/hdr.frag`, `scenebasic_uniform.cpp`

A night-vision mode can be toggled during gameplay. This applies a green-tinted image-intensifier style effect in post-processing, with brightness remapping, grain, and scanline treatment.

This was added as an additional visual mode to make the project more interactive and to demonstrate further control over the post-processing pipeline.

---

## Week 10 — Cook-Torrance Metallic/Roughness Shading

**Files:** `shader/platform.frag`, `SceneWorld.cpp`, `SceneShip.cpp`, `SceneMeshes.cpp`

The main scene-material shader uses a Cook-Torrance-style metallic/roughness model with:

- GGX normal distribution
- Schlick Fresnel approximation
- Smith-style geometry term

Different material settings are assigned to different scene objects so that platforms, debris, the ship, and asteroids respond differently to light.

This improves material definition and surface response compared with a simpler Blinn-Phong-only approach and helps the scene feel more visually varied and physically grounded.

---

## Research — Black Hole Visualisation

**Files:** `shader/blackhole.frag`, `shader/disk.frag`  
**Papers:**
- James, O., von Tunzelmann, E., Franklin, P. and Thorne, K.S. (2015). *Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar.* Classical and Quantum Gravity, 32(6), 065001. doi:10.1088/0264-9381/32/6/065001
- McEwan, I., Sheets, D., Richardson, M. and Gustavson, S. (2012). *Efficient Computational Noise in GLSL.* Journal of Graphics Tools, 16(2).

Both papers are cited inline in the shader source at every point of use.

The black hole visualisation implements specific equations from the James et al. paper rather than using generic glow effects. The goal was to produce a physically grounded real-time approximation that preserves the key visual signatures of Schwarzschild black hole lensing while remaining fast enough to run alongside the full scene — particles, shadows, PBR, UI, and audio all active simultaneously.

---

### Schwarzschild Shadow Boundary — `blackhole.frag`

The dark shadow of a black hole is bounded by the critical impact parameter: the smallest angular separation from the black hole at which a photon can escape rather than spiral inward. For a Schwarzschild black hole this is:

**b_crit = (3√3 / 2) · r_s** *(James et al. 2015, Eq. 8)*

This is implemented as `shadowEdge` in `blackhole.frag`. The constant `2.598` used to recover the Schwarzschild radius from the shadow size is exactly `3√3 / 2`, encoding Eq. 8 directly:

```glsl
// blackhole.frag, line 97
float shadowEdge = clamp(0.268 * (bhRworld / D), 0.05, 0.85);

// blackhole.frag, line 146 — Rs recovered via Eq. 8 (2.598 = 3√3/2)
float Rs = (shadowEdge * D) / 2.598;
```

---

### Ray Deflection — `blackhole.frag`

Photons that escape the shadow region are deflected by the black hole's gravity before reaching the camera. The analytical weak-field deflection angle from the paper is:

**α = 2 r_s / (b · D)** *(James et al. 2015, Eq. 14)*

This is applied to each escaped ray's exit direction before sampling the skybox cubemap, producing the characteristic stretching and warping of background stars around the shadow edge:

```glsl
// blackhole.frag, lines 146–148
float alpha2 = min(2.0 * Rs / max(sinA * D, 0.001), 3.14159265);
d = toFrag * cos(alpha2) + perp * sin(alpha2);
```

---

### Photon Ring — `blackhole.frag`

Just outside the shadow boundary, photons that have orbited the black hole before escaping produce a thin bright ring. The paper describes this using a combined Lorentzian and Gaussian intensity profile (§5). Both the primary and secondary photon rings are implemented:

```glsl
// blackhole.frag, lines 167–174
float rg1Peak = exp(-ringT * ringT * 1.4);              // Gaussian peak
float rg1Lor  = 1.0 / (1.0 + ringT * ringT * 0.35);    // Lorentzian spread
col += mix(vec3(3.5,1.2,0.08), vec3(9.0,6.5,2.8), rg1Peak)
     * rg1Lor * rg1Peak * shimmer2 * dop2 * 10.0;

// Secondary photon ring (narrower, dimmer)
float ringT2 = (bExcess - ringWidth * 0.28) / (ringWidth * 0.20);
col += vec3(5.0,2.4,0.40) * exp(-ringT2 * ringT2) * dop2 * shimmer2 * 4.5;
```

A ghost disk image — the lensed view of the far side of the accretion disk that becomes visible near the shadow boundary — is also rendered as a separate arc (`blackhole.frag`, lines 177–202).

---

### Relativistic Doppler Beaming — `disk.frag`

The most recognisable feature of the *Interstellar* Gargantua render is the asymmetric brightness of the accretion disk: one side is dramatically brighter than the other. This is caused by relativistic Doppler beaming — material orbiting toward the observer is blue-shifted and intensified; material orbiting away is red-shifted and dimmed.

The paper defines the Keplerian orbital velocity as:

**β = v_K / c ≈ 0.58 / √r** *(James et al. 2015, approximation to Eq. 4)*

The Doppler factor is:

**δ = √( (1 + β · n̂) / (1 − β · n̂) )** *(James et al. 2015, §4, Eq. 9)*

And the beaming intensity boost follows:

**I ∝ δ⁴** *(James et al. 2015, §4, Eq. 10)*

All three equations are implemented directly in `disk.frag`:

```glsl
// disk.frag, line 155 — β ≈ 0.58/√r  (James et al. Eq. 4 approximation)
float beta    = clamp(0.58 / max(sqrt(radius), 0.45), 0.0, 0.78);

// disk.frag, line 157 — Doppler factor δ  (James et al. Eq. 9)
float doppler = sqrt((1.0 + beta*vDotCam) / max(0.001, 1.0 - beta*vDotCam));

// disk.frag, line 159 — Beaming intensity ∝ δ⁴  (James et al. Eq. 10)
float beaming = pow(doppler, 4.0);
```

The same Doppler computation is also applied inside the shadow region of `blackhole.frag` (lines 113–116) and on the ghost disk arc (lines 194–197), using β values consistent with the orbital radii in each zone.

---

### Keplerian Rotation and ISCO — `disk.frag`

The disk co-rotates at Keplerian angular speed (Ω ∝ r^{-3/2}, §3.1 of the paper), implemented as the `spinAngle` used to co-rotate the noise sampling frame so that plasma filaments appear to orbit rather than drift:

```glsl
// disk.frag, lines 110–111 — Keplerian Ω ∝ r^{-3/2}
float spinRate  = 0.62 / max(radius * radius, 0.30);
float spinAngle = angle - spinRate * uTime;
```

The innermost stable circular orbit (ISCO) — the closest orbit at which matter remains stable before falling inward — is marked by a white-hot ring at the disk's inner edge, with a sharp emission spike and a wider capture glow:

```glsl
// disk.frag, lines 181–186
float captureGlow = exp(-pow((radius - innerEdge) * 60.0,  2.0));  // broad hot region
float iscoRing    = exp(-pow((radius - innerEdge - 0.02) * 130.0, 2.0));  // sharp ring
```

---

### Simplex Noise Plasma — `blackhole.frag`, `disk.frag`

The accretion disk plasma turbulence and the inner-shadow glow texture are produced using Simplex 3D noise from McEwan et al. (2012). The `permute` and `taylorInvSqrt` functions are taken verbatim from the paper's reference implementation (MIT licence, github.com/ashima/webgl-noise) and cited at the point of use in both shader files. The noise is sampled in a co-rotating frame at three octave scales to build layered plasma filament structure, with alternating time directions per octave to avoid axis-aligned banding:

```glsl
// disk.frag, lines 118–124 — three-octave co-rotating plasma
float t1 = snoise01(nPos*1.6 + vec3(0.0,        uTime*0.09, 0.0));
float t2 = snoise01(nPos*3.8 + vec3(uTime*0.04, 0.0,        uTime*0.025));
float t3 = snoise01(nPos*7.2 + vec3(0.0,       -uTime*0.06, uTime*0.030));
float t4 = snoise01(nPos*14.0+ vec3(uTime*0.08,  uTime*0.04, 0.0));
float plasma = clamp(t1*0.44 + t2*0.30 + t3*0.16 + t4*0.10, 0.0, 1.0);
```

---

### Why This Qualifies as a Research Implementation

The implementation goes beyond using the paper as a visual reference. Specific numbered equations from James et al. (2015) — Eq. 4, Eq. 8, Eq. 9, Eq. 10, Eq. 14, and the §5 photon ring profile — are each implemented as distinct, identifiable lines of GLSL code with the equation number cited in the comment directly above. The McEwan et al. noise functions are included verbatim with attribution. Every research-derived section of both shaders includes an inline citation linking the code back to the source.

The main engineering trade-off was accuracy versus real-time performance. A full numerical geodesic integration of the Kerr metric would reproduce the effect exactly but is far too expensive for interactive use. The approach taken — analytical impact-parameter shadow boundary, first-order deflection angle, and Doppler beaming with Keplerian velocity — preserves the three most visually dominant signatures of the paper (shadow size, photon ring structure, and disk brightness asymmetry) while running at interactive frame rates alongside the rest of the scene.

---

## Gamification

The project is designed as a playable scene rather than a static graphics demo.

Implemented gameplay elements include:

- mission objective: collect all energy cells and return
- multiple relay platform types
- EVA / cockpit mode switching
- tether danger and recovery minigame
- boost resource management
- fail state and reset sequence
- score saving and leaderboard
- staged HUD messaging / narrative progression

These mechanics help connect the rendering systems to an actual gameplay loop with challenge, structure, and persistence.

---

## Aesthetics and Presentation

The visual presentation aims for a stylised cinematic sci-fi look inspired by space-survival and black-hole visual design.

Presentation features include:

- animated main menu
- loading screen
- HUD overlays
- cinematic fail sequence
- platform colour coding by zone type
- post-processing modes for bloom, film look, and night vision
- space-dust haze and distortion near danger zones

A major goal of the project was to ensure that the technical rendering features also contributed to a distinctive and coherent visual identity.

---

## Project Structure

| File | Purpose |
|---|---|
| `main.cpp` | Application entry point |
| `scenebasic_uniform.h/.cpp` | Main scene update, render, and input orchestration |
| `SceneMeshes.cpp` | Geometry creation and mesh support |
| `SceneWorld.cpp` | Main world rendering |
| `SceneShip.cpp` | Ship rendering and related scene objects |
| `SceneParticles.cpp` | Particle spawning, update, and rendering |
| `SceneTether.cpp` | Tether simulation and minigame systems |
| `SceneHUD.cpp` | HUD, overlays, menus, and win/lose screens |
| `SceneMainMenu.cpp` | Main menu visuals |
| `Camera.h/.cpp` | Camera system |
| `GameAudio.h` | Audio manager |
| `LensingDebug.h/.cpp` | Debug support for lensing effect |

---

## Render Pipeline Summary

Each frame, the project broadly follows this order:

1. Render shadow depth map
2. Render main 3D scene into HDR framebuffer
3. Apply bloom blur passes
4. Composite final HDR image
5. Draw HUD, menus, and overlay elements

The main 3D scene includes the skybox, black hole, accretion disk, platforms, ship, particles, tether, and collectable objects.

---

## Software Engineering Notes

Key implementation decisions include:

- splitting large scene logic across multiple `.cpp` files by responsibility
- keeping shared constants in one place
- using cached values where possible to avoid unnecessary repeated work
- structuring the project around a central scene class and clear per-system helpers
- keeping the code compatible with the COMP3015 template structure

The main aim here was to keep the project readable and maintainable despite growing from a smaller CW1 prototype into a more feature-rich CW2 submission.

---

## Performance and Technical Implementation

The project was designed to combine multiple rendering techniques while remaining suitable for real-time execution inside a playable scene.

Key implementation choices included:

- using instanced rendering for particle systems to reduce draw-call overhead
- separating the render pipeline into shadow, HDR scene, blur, composite, and HUD stages
- using a separable blur for bloom rather than a more expensive full-kernel approach
- structuring gameplay and rendering systems across multiple source files so that scene complexity remained manageable
- keeping the project compatible with the COMP3015 template structure while extending it significantly

During testing, the project successfully initialised a 4096×4096 PCF shadow map and a particle setup containing approximately 2200 active particles in the tested scene configuration. Development and testing were carried out on an AMD Radeon RX 5700 XT with OpenGL 4.6 support.

The aim was not only to add advanced features, but to integrate them in a way that remained stable, maintainable, and visually coherent in a real-time application.

---

## Evaluation

The strongest aspect of the project is the way multiple graphics techniques are combined into a single coherent interactive scene rather than being demonstrated as isolated technical exercises.

What worked particularly well:

- integrating advanced rendering with an actual gameplay loop
- creating a distinctive visual identity centred on the black hole and accretion disk
- using UI, post-processing, and feedback systems to reinforce challenge and atmosphere
- extending the original CW1 concept into a more substantial and better-structured CW2 project
- connecting technical features to player-facing outcomes such as danger feedback, tether strain, navigation, and progression

A further strength of the project is that the major rendering features serve more than one purpose. For example, the particle system improves atmosphere and scale, the tether geometry shader contributes both visually and mechanically, and post-processing is used for cinematic presentation as well as gameplay state feedback.

The main limitations of the current implementation are that the analytical deflection model is a first-order weak-field approximation rather than a full geodesic integration, scene hazard variety could be expanded further, and there is still room for deeper profiling and optimisation work. If developed further, the next steps would be to improve accuracy in the near-horizon region using a numerical approach, increase gameplay variety across platform zones, and introduce more formal performance measurement.

Overall, the project is most successful as a research-informed real-time graphics showcase that combines physically grounded rendering, interaction, feedback, and presentation into a unified portfolio-style submission.

---

## References

### Academic / Technical References

- James, O., von Tunzelmann, E., Franklin, P. and Thorne, K.S. (2015). *Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar.* Classical and Quantum Gravity, 32(6), 065001. doi:10.1088/0264-9381/32/6/065001
- McEwan, I., Sheets, D., Richardson, M. and Gustavson, S. (2012). *Efficient Computational Noise in GLSL.* Journal of Graphics Tools, 16(2). github.com/ashima/webgl-noise
- Narkowicz, K. (2015). *ACES Filmic Tone Mapping Curve.* https://knarkowicz.wordpress.com/2016/01/06/
- Perlin, K. (1985). *An image synthesizer.* SIGGRAPH '85.
- Jakobsen, T. (2001). *Advanced Character Physics.*
- Van Ess, J. (2012). *Real-time Night Vision Rendering.* GPU Pro 3, AK Peters.
- de Vries, J. *LearnOpenGL.* https://learnopengl.com

### Asset Credits

**Skybox:** Jettelly space skybox  
**Platform texture:** Poly Haven – blue metal plate  
**Energy cell model:** original Blender model created by me

---

## AI Usage Declaration

Generative AI tools were used in a permitted way consistent with the module guidance.

AI-assisted use included:

- debugging OpenGL and shader issues
- explaining rendering techniques
- helping structure documentation
- supporting iteration and code refinement
  <img width="813" height="548" alt="image" src="https://github.com/user-attachments/assets/37f3b229-d1c3-45cb-9835-6db86958e22c" />

  <img width="938" height="303" alt="image" src="https://github.com/user-attachments/assets/8daba61c-84ba-43b7-b115-bd1778cffa4f" />

  <img width="814" height="637" alt="image" src="https://github.com/user-attachments/assets/beb0c2d4-301b-4c39-9632-c2de2adaf460" />

  <img width="923" height="741" alt="image" src="https://github.com/user-attachments/assets/9b5671a9-7b44-4d17-9adc-87f6126de35d" />


A signed declaration form and AI transcript evidence are included with the submission.

---

<img width="787" height="383" alt="image" src="https://github.com/user-attachments/assets/50a4d9e8-0896-4a73-b6f6-5629a3476cae" />
