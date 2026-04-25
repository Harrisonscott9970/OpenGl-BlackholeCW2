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

The screenshots below show the project’s key presentation and gameplay systems in context.

### Main Gameplay View
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

A shadow map is rendered from the light’s point of view before the main scene pass.

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

The procedural asteroid / environmental motion work was intentionally carried forward into CW2 as part of the noise-based feature set rather than being fully claimed in CW1. In this submission, that procedural variation is treated as part of the project’s broader environmental design and scene animation strategy.

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

## Research-Inspired Black Hole Visualisation

**Files:** `shader/blackhole.frag`, `shader/disk.frag`, `shader/particles.frag`

The black hole is the visual centrepiece of the project. Its appearance was informed by black hole visualisation research and cinematic references such as *Interstellar*, but the coursework implementation is a real-time stylised approximation rather than a physically exact astrophysical simulation.

The implementation focuses on creating a recognisable black hole using a set of layered visual cues:

- a central dark shadow region
- a bright photon-ring-style highlight
- asymmetric brightness / colour emphasis
- distortion of the surrounding environment
- integration with the accretion disk and surrounding particles

The research-inspired aspect of the work is the lensing-style treatment of the background and the visual emphasis placed around the black hole boundary. In a full physical simulation, light bending and relativistic effects would require much more expensive numerical treatment. For this coursework project, those ideas were simplified into a real-time shader suitable for interactive rendering.

This trade-off was deliberate. The goal was to preserve the recognisable visual language of black hole lensing while keeping the effect performant enough to integrate into a wider gameplay scene with post-processing, particles, shadows, UI, and input systems active at the same time.

What makes this feature important in the context of the project is not only the shader itself, but also its integration into the wider scene. The black hole is not treated as an isolated graphics experiment: it directly shapes the project’s atmosphere, player danger feedback, scene composition, and overall identity.

This was one of the main areas in which I aimed to move beyond a standard lab-style implementation and instead produce a more distinctive research-informed visual centrepiece.

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

The main limitations of the current implementation are that the black hole remains a stylised approximation rather than a full physically-based simulation, scene hazard variety could be expanded further, and there is still room for deeper profiling and optimisation work. If developed further, the next steps would be to improve physical grounding in the black hole model, increase gameplay variety across platform zones, and introduce more formal performance measurement.

Overall, the project is most successful as a stylised real-time graphics showcase that combines rendering, interaction, feedback, and presentation into a unified portfolio-style submission.

---

## References

### Academic / Technical References

- James, O., von Tunzelmann, E., Franklin, P. and Thorne, K.S. (2015). *Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar.* Classical and Quantum Gravity, 32(6), 065001.
- Narkowicz, K. (2015). *ACES Filmic Tone Mapping Curve.*
- Perlin, K. (1985). *An image synthesizer.*
- Jakobsen, T. (2001). *Advanced Character Physics.*
- Van Ess, J. (2012). *Real-time Night Vision Rendering.*
- LearnOpenGL.

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

A signed declaration form and AI transcript evidence are included with the submission.
