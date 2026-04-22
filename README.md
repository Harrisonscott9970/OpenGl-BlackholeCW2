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

This project extends my CW1 black hole scene into a more complete interactive experience built around multiple advanced rendering techniques from Weeks 5–10 of the module. The player pilots a spacecraft near TON 618, leaves the ship in EVA mode to recover energy cells from relay platforms, and must return safely before being pulled into the black hole.

The focus of the coursework is the integration of several advanced graphics features into one coherent scene and gameplay loop, including:

- HDR tonemapping and bloom
- shadow mapping with PCF filtering
- instanced particles
- noise-driven procedural environmental motion and variation
- Cook-Torrance-style metallic/roughness shading
- a geometry-shader tether ribbon
- a research-inspired black hole visualisation effect

These rendering systems are combined with HUD systems, menu screens, a tether recovery minigame, score persistence, and cinematic fail / win presentation so that the project functions as a playable and polished graphics portfolio piece rather than a disconnected collection of effects.

---

## New Contributions Beyond CW1

Although this project builds directly on my CW1 black hole scene, the CW2 submission is a substantial extension rather than a simple resubmission.

New or significantly expanded CW2 contributions include:

- shadow mapping with PCF filtering
- geometry-shader rendering for the EVA tether ribbon
- instanced particle rendering for environmental effects
- noise-based procedural environmental motion and variation
- Cook-Torrance-style metallic/roughness shading
- expanded gameplay systems including EVA mode, tether danger/recovery, score persistence, and cinematic fail / win presentation
- a broader and more modular render pipeline structure across multiple scene systems

This allows the original CW1 idea to be developed into a more complete and technically ambitious interactive project.

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

The black hole visuals are informed by black hole visualisation research and cinematic references such as *Interstellar*, but the coursework implementation is a real-time stylised approximation rather than a physically exact simulation.

The shader focuses on effects such as:

- a dark shadow region
- a bright photon-ring-style highlight
- asymmetric brightness / colour emphasis
- distortion of the surrounding environment

The research-inspired part of the implementation is the lensing-style visual distortion, ring emphasis, and asymmetric visual treatment around the black hole. The wider gameplay systems, scene composition, post-processing pipeline, and integration into the overall project are my own coursework implementation decisions.

The aim was to create a recognisable and convincing black hole effect within the constraints of real-time coursework rendering while still grounding the result in research-inspired ideas.

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

## Evaluation

I believe the strongest part of the project is the way multiple graphics techniques are combined into one coherent interactive scene rather than being demonstrated separately.

What worked well:

- integrating advanced rendering with gameplay
- creating a distinctive visual identity
- building a more polished CW2 version of the original CW1 idea
- linking rendering systems to player feedback and challenge

What could be improved further:

- even more variation in platform layouts and hazards
- more balancing of the difficulty curve
- further refinement of the black hole effect toward a more physically grounded result
- additional optimisation and profiling work

Overall, the project succeeds best as a stylised and playable graphics showcase that combines rendering, interaction, and presentation into one portfolio-style submission.

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
