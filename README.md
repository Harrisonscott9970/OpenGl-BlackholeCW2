# TON 618: Stellar Expedition — COMP3015 CW2

**Student:** Harrison Scott  
**Student ID:** 10805603  
**Module:** COMP3015 – Games Graphics Pipelines  
**Module Leader:** Dr Ji-Jian Chin  
**Repository:** https://github.com/Harrisonscott9970/OpenGl-BlackholeCW2  
**Video Demo:** [your YouTube URL]

---

## Project Overview

TON 618: Stellar Expedition is an OpenGL / GLSL real-time graphics application created for COMP3015 CW2. It is a direct evolution of the CW1 black hole scene, rebuilt from the ground up around seven advanced rendering features from Weeks 5–10 of the module.

The player pilots a spacecraft in orbit around TON 618 — a supermassive black hole. The objective is to fly out from the home platform, collect energy cells from four zone types of satellite platform (HomeRelay, StableRelay, DamagedRelay, HazardRelay), and return safely before the black hole consumes you.

The project demonstrates a **full production rendering pipeline**: physically based shading, instanced particle systems, geometry shader ribbon rendering, PCF shadow maps, post-process HDR with filmic tonemapping and bloom, FBM noise-driven motion, and a research-backed gravitational lensing effect — all integrated into a playable game loop with a menu, leaderboard, narrative progression, and cinematic fail sequence.

---

## Build Information

| Item | Detail |
|---|---|
| IDE | Visual Studio 2022 |
| OpenGL Version | 4.3 Core Profile |
| Template Base | COMP3015 Lab 1 template |
| Libraries | GLAD, GLFW, GLM, stb\_image, irrKlang |

> **No Assimp dependency.** All geometry (platforms, ship, cell, asteroids, debris) is generated procedurally or loaded through a self-contained OBJ parser (`loadOBJMesh()` in `SceneMeshes.cpp`) using only `std::ifstream`. There are no external model-loading DLLs.

---

## Submission / Opening Instructions

To run the project, either:

- go to `CompProject\x64\Debug` and launch `project_template.exe`, or
- go to `CompProject`, open `project_template.sln`, and run it with **Local Windows Debugger** in Visual Studio.

The final submission zip includes:

- the full Visual Studio project
- the executable version
- all required assets, shaders, and media files
- this README
- the signed Generative AI declaration form with AI prompt transcripts per feature

---

## Controls

| Key / Input | Action |
|---|---|
| W / A / S / D | Move (cockpit: fly ship / EVA: float) |
| Mouse | Look / aim |
| **X** | Toggle EVA / cockpit mode |
| **Left Mouse** | Collect nearby energy cell (EVA only) |
| **Shift** | Boost (drains boost energy; auto-recharges) |
| **A / D** (during tether minigame) | Align marker with stability window |
| **N** | Toggle night-vision mode (phosphor green) |
| **H** | Toggle HDR tonemapping |
| **B** | Toggle bloom |
| **F** | Toggle film mode (grain + scanlines + desaturation) |
| **Q / E** | Decrease / increase exposure |
| **G** | Reset mission |
| **Escape** | Quit |

---

## Objective & Game Loop

1. Launch from the **home platform** (cyan, at world origin) into open space
2. Fly to satellite platforms at **ring radii 85 / 180 / 310 units** from the black hole
3. **Collect all energy cells** (left-click when nearby in EVA mode)
4. Return the cells to the **home platform** to win
5. If you drift too far from the ship in EVA mode, the **tether minigame** triggers — press A / D to align the oscilloscope marker and reel yourself back in
6. If you cross the event horizon, a **cinematic BH-fall sequence** plays (red-shift, tidal vignette, chromatic aberration) before the mission resets
7. High scores are saved to `scores.txt`; the **leaderboard** shows your top-5 runs

---

## Feature Coverage

### Week 5 — HDR Tonemapping + Gaussian Bloom

**Files:** `shader/hdr.frag`, `shader/blur.frag`, `scenebasic_uniform.cpp` (`renderBloom()`, `setupHDRFramebuffer()`)

The scene renders entirely into a floating-point HDR framebuffer. A 10-pass separable Gaussian bloom is computed via a ping-pong FBO pair before the final composite. The HDR composite shader (`hdr.frag`) applies:

- **ACES filmic tonemapping** — Narkowicz (2015) fitted curve constants (a=2.51, b=0.03, c=2.43, d=0.59, e=0.14)
- Shadow crush and warm highlight push for a deep-space cinematic grade
- Gentle S-curve contrast
- Film grain + CRT scanlines (film mode, toggle F)
- Vignette framing the black hole at screen centre
- Edge chromatic aberration
- Dust proximity UV warp + 9-tap weighted scatter blur (`uDustNearby` uniform)
- Gravitational proximity distortion rings (`uBHProximity`)
- BH-fall red-shift and tidal vignette (`uBHFallDistort`)
- Exposure control (Q/E keys)

> Reference: Narkowicz, K. (2015). *ACES Filmic Tone Mapping Curve.* https://knarkowicz.wordpress.com/2016/01/06/

---

### Week 6 — Geometry Shader Tether Ribbon

**Files:** `shader/tether.vert`, `shader/tether.geom`, `shader/tether.frag`, `SceneTether.cpp`

The EVA tether rope is simulated as a 24-segment Verlet chain (Jakobsen 2001 GDC) on the CPU. The geometry shader extrudes the resulting `GL_LINE_STRIP` into a smooth **camera-facing quad strip**:

```glsl
// tether.geom — core billboarding
vec3 right = normalize(cross(dir, toEye));
```

The ribbon half-width is driven by `uHalfWidth`, which is animated via `tetherWarning` (0→1 as the player approaches maximum tether distance) so the rope visually pulses orange-red when under tension. The fragment shader applies a glow gradient across the ribbon width.

> Reference: Jakobsen, T. (2001). *Advanced Character Physics.* GDC 2001.

---

### Week 7 — Instanced Particle System (Dual-type)

**Files:** `shader/particles.vert`, `shader/particles.frag`, `SceneParticles.cpp`

2,200 billboard quads are drawn in a **single `glDrawArraysInstanced` call**. Each instance carries 7 floats: `spawnPos (xyz)`, `age`, `size`, `seed`, `type`. Billboarding is done in the vertex shader from the camera right/up vectors extracted from the view matrix.

**Type 0 — Accretion particles (1,800):**
- Schwarzschild-inspired gravity term (`pow(age, 1.6)`) pulls the particle toward the black hole as it ages
- 4-octave FBM turbulence (Perlin 1985) adds lateral swirl
- Temperature gradient colour: cold blue → orange → white-hot (matches the Keplerian disk profile from James et al. 2015)

**Type 1 — Space dust (400):**
- Spawned on four concentric distance rings (260 / 420 / 580 / 740 units)
- Slow volumetric FBM drift, never infalling
- Additive blending builds a visible blue-grey haze through the playfield
- Camera proximity (`dustNearbyDensity`) drives the `uDustNearby` HDR overlay

> Reference: Perlin, K. (1985). *An image synthesizer.* SIGGRAPH 1985.

---

### Week 8 — Shadow Map (PCF)

**Files:** `shader/shadow_depth.vert`, `shader/shadow_depth.frag`, `shader/platform.frag` (`calcShadow()`), `scenebasic_uniform.cpp` (`setupShadowMap()`, `renderShadowPass()`)

A 4096×4096 depth-only framebuffer is rendered from a directional light before the main pass. The `lightSpaceMatrix` (orthographic light projection × light view) is passed to the platform shader.

The `calcShadow()` function in `platform.frag` performs **5×5 Percentage Closer Filtering (PCF)** — 25 depth comparisons, each offset by a texel, averaged for soft shadow edges. Slope-scaled depth bias (`bias = max(0.005, 0.02 * (1 - NdotL))`) eliminates shadow acne on angled surfaces without over-biasing.

This is the **declared Lecture 5–10 shader** for the pass condition.

---

### Week 9 — FBM Noise (Particle Motion + Plasma Turbulence)

**Files:** `shader/particles.vert` (`ptFbm()`), `shader/blackhole.frag`, `shader/disk.frag`

4-octave **value-noise FBM** (`ptFbm()` in `particles.vert`) uses a 2D rotation matrix between octaves to break axis-aligned banding artifacts. The same FBM concept appears in two further shaders:

- `blackhole.frag` — plasma filament distortion around the event horizon
- `disk.frag` — turbulence overlay on the accretion disk surface

This means the noise technique is applied across three different visual systems rather than a single isolated use.

> Reference: Perlin, K. (1985). *An image synthesizer.* SIGGRAPH 1985.

---

### Week 9 — Night Vision Mode (Phosphor-Green Image Intensifier)

**Files:** `shader/hdr.frag` (lines 181–215, `uNightVision` uniform), `scenebasic_uniform.cpp` (N key toggle)

A full post-process night-vision pipeline in `hdr.frag`, activated by pressing **N**:

1. **ITU-R BT.601 luminance** extraction from the tonemapped frame
2. **Power-curve amplification** (`pow(lum, 0.55) * 1.30`) — lifts dim sources (deep space, particles) without blowing bright ones, simulating the image-intensifier tube
3. **P31 phosphor green remap** (peak ~530 nm): `r * 0.14, g * 1.00, b * 0.06`
4. **Photon shot-noise grain** (`h21` hash, McGuire 2020 JCGT)
5. **CRT scanlines** (every other pixel row, 4% dimmer)
6. **Green bloom pass** re-added on bright objects (photon ring, energy cells)

Skips the dust-haze overlay automatically — NV luminance amplification already makes dust quads dominate without the extra HDR blend.

> Reference: Van Ess, J. (2012). *Real-time Night Vision Rendering.* GPU Pro 3, AK Peters.

---

### Week 10 — PBR Cook-Torrance

**Files:** `shader/platform.frag` (full PBR BRDF), `SceneMeshes.cpp`, `SceneShip.cpp`, `SceneWorld.cpp`, `scenebasic_uniform.cpp`

Full **metallic/roughness PBR** in `platform.frag`, applied to all scene geometry. The BRDF consists of:

- **GGX normal distribution** — `D = a² / (π · ((NdotH² · (a²-1) + 1)²))`
- **Smith-correlated geometry masking-shadowing** — `G = G_SchlickGGX(NdotV) · G_SchlickGGX(NdotL)`
- **Schlick Fresnel approximation** — `F = F0 + (1 - F0) · (1 - HdotV)⁵`
- Energy-conserving specular/diffuse split via `kD = (1 - F) · (1 - metallic)`

Per-surface `uMetallic` / `uRoughness` values are sent from C++ for every draw call:

| Surface | Metallic | Roughness | Rationale |
|---|---|---|---|
| HomeRelay platform | 0.22 | 0.42 | Maintained aluminium |
| DamagedRelay platform | 0.06 | 0.82 | Oxidised, corroded |
| HazardRelay platform | 0.03 | 0.94 | Heavily degraded |
| StableRelay platform | 0.15 | 0.55 | General station metal |
| Asteroids | 0.00 | 0.92 | Rock (non-conductive) |
| Beacon antenna | 0.45 | 0.28 | Polished steel |
| Debris props | 0.18 | 0.78 | Oxidised scrap |
| Ship hull | 0.08 | 0.44 | Aerospace ceramic |

---

## Advanced Research Feature

**Paper:** James, O., von Tunzelmann, E., Franklin, P. and Thorne, K.S. (2015). *Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar.* Classical and Quantum Gravity, 32(6), 065001. DOI: 10.1088/0264-9381/32/6/065001

Four distinct aspects of the paper are implemented across three shader files:

| Concept from paper | Implementation | Shader |
|---|---|---|
| Shadow boundary at critical impact parameter b_crit = 3√3/2 · M | Fragments inside b_crit rendered as the black shadow disk | `blackhole.frag` |
| Photon ring emission at b ≈ b_crit | Thin bright ring with angular-velocity brightness modulation | `blackhole.frag` |
| Doppler beaming — approaching side brighter | Asymmetric ring brightness based on orbital direction | `blackhole.frag` |
| Keplerian temperature gradient T ∝ r^{−3/4} | Cold blue → orange → white-hot particle colour ramp matching paper Fig. 2 Stefan-Boltzmann profile | `particles.frag` |
| Gravitational light bending | Screen-space UV warp approximating Schwarzschild radius deflection | `blackhole.frag` |

---

## Aesthetics

- **Cinematic ACES grade** — deep-black crush, warm orange highlights on the disk, cold interstellar space tones
- **Animated main menu** — CPU-rasterized rotating black hole with Doppler beaming, twinkling star field, pulsing title text, leaderboard and settings pages
- **Loading screen** — animated spaceship riding a progress bar with step-reveal loading messages
- **BH-fall cinematic** — extreme chromatic aberration, red-shift, and tidal edge vignette ramp over 4 seconds
- **Space-dust haze** — flying through dust clouds produces a cold blue-grey screen-space blur and scatter overlay
- **Zone visual identity** — HomeRelay (cyan), StableRelay (blue-cyan), DamagedRelay (orange), HazardRelay (red) with per-type glow and debris density
- **Dust cluster HUD markers** — compass-style indicators point to the nearest dust cloud positions computed each frame from the particle system

---

## Gamification

| Element | Implementation |
|---|---|
| **Objective** | Collect all energy cells from satellite platforms and return to the home platform |
| **Zone types** | 4 archetypes — HomeRelay / StableRelay / DamagedRelay / HazardRelay — with distinct visual identity, debris density, and danger bias |
| **Tether minigame** | Oscilloscope rhythm mechanic: align a sweeping marker with the stability window; hit window narrows with each press; 3 successes reels player to ship |
| **Danger system** | `dangerLevel` (0→1) scales particle brightness, exposure, BH-pull, and HDR distortion as player approaches the black hole |
| **Boost mechanic** | Shift-boost with drain/recharge cycle; auto-locks after depletion until 25% recovered |
| **Fail state** | Crossing event horizon triggers 4-second red-shift cinematic then mission reset |
| **Win state** | Returning all cells to home platform — score saved to `scores.txt` |
| **Score + combo** | Score per cell; combo multiplier for rapid collections |
| **High-score persistence** | `scores.txt` stores top-5 entries; leaderboard page on main menu |
| **Narrative** | 5-stage HELIO ARRAY COLLAPSE story shown as HUD overlays at score thresholds |
| **Time dilation** | `timeDilationFactor` scales difficulty tier dynamically with proximity and score |

---

## Project File Structure

| File | Purpose |
|---|---|
| `main.cpp` | Application entry point |
| `scenebasic_uniform.h/.cpp` | Main scene class: init, update, render, input, game state |
| `SceneMeshes.cpp` | Geometry generation: sphere, disk, platform, ship, OBJ loader |
| `SceneWorld.cpp` | World rendering: black hole, disk, skybox, cell, asteroids, shadow pass |
| `SceneShip.cpp` | Ship exterior, beacon towers, debris props, cockpit glass |
| `SceneParticles.cpp` | Dual-type instanced particle system: spawn, update, render, dust haze |
| `SceneTether.cpp` | Verlet rope simulation, geometry shader ribbon, tether minigame logic |
| `SceneHUD.cpp` | All 2D overlay: HUD compass, menus, loading screen, win/lose screens |
| `SceneMainMenu.cpp` | Animated main menu rendering (CPU-rasterized BH, star field) |
| `Camera.h/.cpp` | FPS-style free-look camera |
| `GameAudio.h` | irrKlang audio manager (danger music, win/lose stings) |
| `LensingDebug.h/.cpp` | Research gravitational lensing debug overlay |
| `shader/blackhole.vert/.frag` | Black hole shadow disk, photon ring, UV warp lensing |
| `shader/disk.vert/.frag` | Procedural accretion disk with FBM plasma turbulence |
| `shader/platform.vert/.frag` | Cook-Torrance PBR: all platforms, asteroids, ship, debris, cells |
| `shader/particles.vert/.frag` | Instanced billboard particle system: dual-type with FBM + temp gradient |
| `shader/tether.vert/.geom/.frag` | Geometry shader tether ribbon with animated glow width |
| `shader/shadow_depth.vert/.frag` | Depth-only shadow map pass |
| `shader/hdr.vert/.frag` | ACES HDR, Gaussian bloom, film mode, night vision, dust haze, BH distortions |
| `shader/blur.frag` | Separable Gaussian blur (bloom ping-pong) |
| `shader/skybox.vert/.frag` | Cubemap skybox environment |
| `media/` | Skybox cubemap faces, platform textures, cell.obj |
| `helper/` | COMP3015 template helper code |

---

## How It Works

The application is structured around a single `SceneBasic_Uniform` class split across multiple `.cpp` files by system area. The render path runs each frame as:

1. **Shadow pass** — render all scene geometry from the light's point of view into a 4096×4096 depth FBO (`renderShadowPass()`)
2. **Main HDR pass** (into floating-point FBO):
   - Skybox
   - Black hole sphere + photon ring (`blackhole.frag`)
   - Accretion disk (`disk.frag`)
   - Platforms, ship, asteroids, cells, debris (`platform.frag` — PBR)
   - Tether rope ribbon (`tether.geom`)
   - Instanced particle system (`particles.vert/.frag`)
3. **Bloom pass** — 10-pass separable Gaussian on a ping-pong FBO pair
4. **HDR composite** — ACES tonemapping, bloom mix, film grade, night vision, dust haze, BH distortions, fade (`hdr.frag`)
5. **2D overlay** — CPU-rasterized HUD, menus, minigame UI (`SceneHUD.cpp`)

---

## Software Engineering Notes

- All geometry is **procedurally generated** or parsed from a self-contained OBJ loader — no Assimp dependency
- Systems are separated into per-area `.cpp` files for readability (`SceneMeshes`, `SceneWorld`, `SceneShip`, `SceneParticles`, `SceneTether`, `SceneHUD`)
- Named constants (`HOME_PLATFORM_Z`, `ROPE_SEGMENTS`, `MAX_PARTICLES`) in the header provide a single source of truth across the codebase
- Per-frame efficiency: `collectedCells` is a cached counter (not an O(n) loop); window title is throttled to 4 Hz; `dustClusterPositions` is pre-computed in `renderParticles()` so `drawHUD()` never re-iterates the full particle array
- Key edge-detection is member-variable-based so `resetGame()` can zero it — no function-local statics that silently persist across resets

---

## References

### Academic / Technical References

- James, O., von Tunzelmann, E., Franklin, P. and Thorne, K.S. (2015). *Gravitational lensing by spinning black holes in astrophysics, and in the movie Interstellar.* Classical and Quantum Gravity, 32(6), 065001. DOI: 10.1088/0264-9381/32/6/065001  
  *Research feature: shadow disk, photon ring, Doppler beaming, Keplerian temperature profile.*

- Narkowicz, K. (2015). *ACES Filmic Tone Mapping Curve.* https://knarkowicz.wordpress.com/2016/01/06/  
  *ACES constants in `hdr.frag` ACESFilm() function.*

- Perlin, K. (1985). *An image synthesizer.* SIGGRAPH 1985.  
  *FBM noise basis in `particles.vert` ptFbm() and disk/blackhole shaders.*

- McGuire, M. (2020). *Hash Functions for GPU Rendering.* JCGT 9(3). https://jcgt.org/published/0009/03/02/  
  *h21 hash function used for film grain and night-vision shot noise in `hdr.frag`.*

- Jakobsen, T. (2001). *Advanced Character Physics.* GDC 2001.  
  *Verlet rope integration in `SceneTether.cpp` — damping and gravity constant rationale.*

- Van Ess, J. (2012). *Real-time Night Vision Rendering.* GPU Pro 3, AK Peters.  
  *Intensity amplification + phosphor colour model in `hdr.frag` night-vision pass.*

- Blinn, J. F. (1977). *Models of light reflection for computer synthesized pictures.* SIGGRAPH 1977.  
  *Blinn-Phong basis underlying the PBR diffuse term.*

- LearnOpenGL. https://learnopengl.com  
  *General reference for OpenGL techniques, framebuffer workflows, shadow mapping, cubemaps.*

---

### Asset Credits

#### Skybox / Space Cubemap

Six-face space skybox (Jettelly):

- `jettelly_space_common_blue_RIGHT/LEFT/UP/DOWN/FRONT/BACK.png`

https://jettelly.com/blog/some-space-skyboxes-why-not

#### Platform Texture

- `blue_metal_plate_diff_4k.jpg`
- `blue_metal_plate_disp_4k.png`

https://polyhaven.com/a/blue_metal_plate

#### Energy Cell Model

- `cell.obj` / `cell.mtl` — original Blender model created by me for use as the collectible pickup object.

---

## AI Usage Declaration

Generative AI tools were used in a permitted way consistent with the module's partnered-use guidance. AI-assisted work included:

- Debugging shader compilation and OpenGL state issues
- Explanation of rendering techniques (PBR BRDF, PCF filtering, geometry shader billboarding)
- Documentation drafting and refinement
- Code-structure suggestions during iteration

A full transcript of AI prompts and responses for each claimed feature is attached to the submission as required by Rule 8 of the CW2 specification.
