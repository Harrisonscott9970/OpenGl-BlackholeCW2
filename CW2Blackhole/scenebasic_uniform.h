#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/glslprogram.h"
#include "GameAudio.h"
#include <glad/glad.h>
#include "Camera.h"
#include "LensingDebug.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

enum class PickupPhase
{
    Idle,
    SpinShrink,
    Poof
};

struct EnergyCell
{
    glm::vec3   position;
    glm::vec3   platformPos;
    bool        collected;

    PickupPhase phase;
    float       phaseTimer;

    float       spinAngle;
    float       cellScale;
    float       orbScale;
    float       poofAlpha;

    static constexpr float SPIN_SHRINK_DURATION = 2.2f;
    static constexpr float POOF_DURATION = 1.2f;

    EnergyCell()
        : position(0.f), platformPos(0.f),
        collected(false), phase(PickupPhase::Idle),
        phaseTimer(0.f), spinAngle(0.f),
        cellScale(1.f), orbScale(0.f), poofAlpha(1.f)
    {
    }
};

struct Asteroid
{
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotationAxis;
    float orbitRadius;
    float angle;
    float angularSpeed;
    float inwardSpeed;
    float heightOffset;
    float tiltAngle;
    float rotationAngle;
    float rotationSpeed;
    float wobblePhase;
    bool  active;

    Asteroid()
        : position(0.f), scale(1.f), rotationAxis(0.f, 1.f, 0.f),
        orbitRadius(0.f), angle(0.f), angularSpeed(0.f), inwardSpeed(0.f),
        heightOffset(0.f), tiltAngle(0.f), rotationAngle(0.f),
        rotationSpeed(0.f), wobblePhase(0.f), active(true)
    {
    }
};

enum class PlatformZoneType
{
    HomeRelay,
    StableRelay,
    DamagedRelay,
    HazardRelay
};

struct PlatformZone
{
    glm::vec3 position = glm::vec3(0.0f);
    PlatformZoneType type = PlatformZoneType::StableRelay;
    glm::vec3 accentColor = glm::vec3(0.15f, 0.55f, 1.0f);
    float glowStrength = 1.0f;
    float localDangerBias = 0.0f;
};

struct BeaconTower
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 glowColor = glm::vec3(0.15f, 0.55f, 1.0f);
    float pulseSpeed = 1.0f;
    float pulseOffset = 0.0f;
    bool damaged = false;
};

struct DebrisProp
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 baseColor = glm::vec3(0.08f, 0.10f, 0.14f);
    glm::vec3 glowColor = glm::vec3(0.0f);
    float rotationSpeed = 0.0f;
    float angle = 0.0f;
    bool hazardLit = false;
};

// ?? Tether Stabilisation Minigame ????????????????????????????????????????????
// Redesigned to match the oscilloscope "stability window" UI from the brief.
// The player watches a wave marker sweep across a waveform and presses A or D
// when it aligns with the highlighted stability window.  Three successful
// alignments trigger the launch sequence.
enum class TetherMinigameState
{
    Inactive,   // Not playing
    Active,     // Waiting for A/D inputs
    Success,    // Launch succeeded
    Fail        // Missed the window
};

struct TetherRhythm
{
    // ?? Marker sweep ??????????????????????????????????????????????????????????
    float   markerPos;          // 0..1 position of the moving marker
    float   markerSpeed;        // how fast it sweeps (units/sec)
    float   markerDir;          // +1 or -1
    // ?? Stability window ??????????????????????????????????????????????????????
    float   zoneCenter;         // centre of the hit zone (0..1)
    float   zoneHalfWidth;      // half-width (narrows with each successful press)
    // ?? Progress ??????????????????????????????????????????????????????????????
    int     pressCount;         // successful presses so far (need 3)
    bool    expectingA;         // alternates: true=A, false=D
    float   launchProgress;     // 0..1 visual fill
    // ?? Feedback flashes ??????????????????????????????????????????????????????
    float   successPulse;       // 0..1, fades out
    float   failFlash;          // 0..1, fades out
    // ?? Oscilloscope waveform ?????????????????????????????????????????????????
    // We store the wave phase so the waveform animates continuously.
    float   wavePhase;          // advances with time
    float   tensionPct;         // 0..1 shown as the TENSION bar
    // ?? Reel-in distance tracker ??????????????????????????????????????????????
    float   reelRemaining;      // metres left to reel in (counts down on success)
};

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;            // blackhole sphere
    GLSLProgram diskProg;        // accretion disk
    GLSLProgram skyboxProg;      // cubemap skybox
    GLSLProgram hdrProg;         // HDR + bloom composite
    GLSLProgram blurProg;        // Gaussian blur for bloom
    GLSLProgram platformProg;    // PBR shader for platforms, ship, debris
    GLSLProgram particleProg;    // Instanced particle shader
    GLSLProgram shadowProg;      // Shadow map depth pass
    GLSLProgram tetherProg;      // Tether rope ribbon using geometry shader

    // Simple OBJ mesh loaded with a custom parser
    struct OBJMesh {
        GLuint vao = 0, vbo = 0, ebo = 0;
        int    indexCount = 0;
        bool   loaded = false;
    };
    OBJMesh cellMesh;   // Blender-exported cell shape (fallback: sphereVAO)

    GLuint sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
    int    sphereIdxCount = 0;

    GLuint diskVAO = 0, diskVBO = 0;
    int    diskVertCount = 0;

    GLuint platVAO = 0, platVBO = 0;
    int    platVertCount = 0;

    // Triangulated ship hull mesh
    GLuint shipVAO = 0, shipVBO = 0;
    int    shipVertCount = 0;

    // Tether rope line geometry (updated each frame in EVA)
    GLuint tetherVAO = 0, tetherVBO = 0;

    // Instanced particle system — 1800 accretion particles + 400 space dust motes
    static constexpr int  MAX_PARTICLES   = 2200;
    static constexpr int  DUST_COUNT      =  400;
    static constexpr int  ACCRETION_COUNT = MAX_PARTICLES - DUST_COUNT; // 1800
    GLuint particleQuadVAO = 0;  // unit quad geometry
    GLuint particleQuadVBO = 0;
    GLuint particleInstVBO = 0;  // per-instance data (streamed each frame)
    // Per-particle CPU state — 7 floats uploaded per instance
    struct Particle {
        glm::vec3 spawnPos;   // initial world position
        float     age;        // 0..1 normalised lifetime
        float     size;       // world-space radius
        float     seed;       // random offset for noise
        float     type;       // 0 = accretion particle, 1 = space dust
        float     lifespan;   // seconds for full 0..1 cycle
    };
    std::vector<Particle> particles;

    // Closest dust cluster positions — updated every frame for the NV HUD markers
    std::vector<glm::vec3> dustClusterPositions;

    GLuint satellitePlatformTex = 0;

    GLuint skyboxVAO = 0, skyboxVBO = 0;
    GLuint skyboxTex = 0;

    GLuint hdrFBO = 0, hdrColorBuf = 0, rboDepth = 0;
    GLuint pingpongFBO[2] = { 0, 0 };
    GLuint pingpongColorbuffers[2] = { 0, 0 };
    GLuint screenVAO = 0, screenVBO = 0;

    // Shadow map (4096x4096, PCF soft edges)
    static constexpr int  SHADOW_MAP_SIZE = 4096;
    GLuint shadowFBO = 0;       // framebuffer for depth-only render
    GLuint shadowMap = 0;       // depth texture (GL_DEPTH_COMPONENT)
    glm::mat4 lightSpaceMatrix{ 1 };  // light projection * light view
    glm::vec3 shadowLightPos{ 0 };    // world position of shadow-casting light

    Camera camera = Camera(glm::vec3(0.0f, 3.5f, 145.0f));

    glm::vec3 lightPos = glm::vec3(0.0f);
    float     lightOrbitAngle = 0.0f;

    bool  hdrEnabled = true;
    bool  bloomEnabled = true;
    bool  filmMode = false;
    float exposure = 0.85f;

    glm::vec3 bhPos = glm::vec3(0.0f, 30.0f, -1350.0f);
    float     bhR = 420.0f;

    std::vector<PlatformZone> platformZones;
    std::vector<BeaconTower> beaconTowers;
    std::vector<DebrisProp> debrisProps;
    std::vector<EnergyCell> energyCells;
    std::vector<Asteroid> asteroids;

    int   asteroidCount = 320;
    float asteroidBeltInnerRadius = 560.0f;
    float asteroidBeltOuterRadius = 980.0f;
    float asteroidBeltHalfHeight = 52.0f;
    float asteroidConsumeRadius = 120.0f;
    float asteroidPullRadius = 460.0f;

    bool  gameWon = false;
    bool  gameLost = false;
    float dangerLevel = 0.0f;
    float dustNearbyDensity = 0.0f;   // 0..1  — dust-haze intensity for HDR overlay
    int   narrativeStage = 0;     // HELIO ARRAY COLLAPSE story progression

    // BH fall cinematic state  (initialised in constructor body)
    bool      fallingIntoBH;
    glm::vec3 bhFallDir;
    float     bhFallT;

    float boostEnergy = 1.0f;
    float boostDrainRate = 0.42f;
    float boostRechargeRate = 0.22f;
    float boostMultiplier = 2.85f;
    bool  boostActive = false;
    bool  boostLocked = false;   // true after depletion; cleared when energy >= 25%
    float boostFlash = 0.0f;

    bool  inCockpit = true;
    glm::vec3 shipPosition = glm::vec3(0.0f, 3.5f, 145.0f);
    float shipReentryDistance = 16.0f;

    float evaMoveMultiplier = 0.70f;
    float tetherMaxDistance = 95.0f;    // tether rope length
    float tetherWarning = 0.0f;
    float tetherLockedDist = 0.0f;  // max dist enforced while minigame is active
    float cockpitPulse = 0.0f;

    float shakeDuration = 0.0f;
    float shakeMagnitude = 0.0f;

    float fadeAlpha = 0.0f;
    float endStateTimer = 0.0f;
    int   fadeState = 0; // 0 none, 1 wait, 2 fade out, 3 fade in

    // NMS-style cockpit glass warp distortion phase
    float glassWarpPhase = 0.0f;

    // Tether rope & minigame
    TetherMinigameState minigameState = TetherMinigameState::Inactive;
    TetherRhythm        tether{};

    // rope sway simulation
    static constexpr int ROPE_SEGMENTS = 24;
    glm::vec3 ropeNodes[ROPE_SEGMENTS + 1] = {};
    glm::vec3 ropeVelocities[ROPE_SEGMENTS + 1] = {};

    // Ship exterior / gameplay presentation
    float shipPulsePhase = 0.0f;

    // CW2 polish systems
    float missionStartTime = 0.0f;
    float missionFinalTime = 0.0f;  // frozen at win so timer doesn't keep running
    int   playerScore = 0;

    // High scores loaded from and saved to scores.txt
    std::vector<std::pair<int,float>> highScores;

    // Main menu
    bool  showMenu = true;      // true = show menu, false = in game
    bool  showLoading = false;  // loading screen between menu and game
    float loadingProgress = 0.0f;
    int   menuSelection = 0;    // 0=Play 1=Leaderboard 2=Settings 3=Exit
    int   menuPage = 0;         // 0=main 1=leaderboard 2=settings
    int   settingsSelection = 0;
    float menuBHAngle = 0.0f;
    float menuTime = 0.0f;
    int   comboCount = 0;
    float comboTimer = 0.0f;
    float nearDiskSlipstream = 0.0f;
    float timeDilationFactor = 1.0f;
    float difficultyTier = 0.0f;
    float pickupPulse = 0.0f;

    // Home platform world position — shared by win-check, spawn, HUD, and shadow pass
    static constexpr float HOME_PLATFORM_X =   0.0f;
    static constexpr float HOME_WIN_Y       =   2.0f;
    static constexpr float HOME_SPAWN_Y     =   3.5f;
    static constexpr float HOME_PLATFORM_Z  = 145.0f;

    int   collectedCells = 0;     // count updated by updatePickupAnimations()
    float titleUpdateTimer = 0.0f;
    float lastHintTime = -10.0f;

    // Per-frame key edge-detect state (stored as members so resetGame() can clear them)
    bool prevX       = false;   // EVA / cockpit toggle
    bool prevH       = false;   // HDR toggle
    bool prevB       = false;   // Bloom toggle
    bool prevF       = false;   // Film-mode toggle
    bool prevQ       = false;   // Exposure decrease
    bool prevEKey    = false;   // Exposure increase
    bool prevG       = false;   // Mission reset
    bool prevLMB     = false;   // Left mouse — cell pickup
    bool prevC       = false;   // Debug: instant collect all
    bool prevTetherA = false;   // Tether minigame A key
    bool prevTetherD = false;   // Tether minigame D key
    bool prevN       = false;   // Night-vision toggle
    bool nightVisionMode = false;
    bool prevZ          = false;   // Shadow toggle
    bool shadowsEnabled = true;

    // Hyperspace jump — spam SPACE 15 times in cockpit, 3 charges per game
    static constexpr int   HYPERSPACE_PRESSES       = 15;
    static constexpr float HYPERSPACE_CHAIN_TIMEOUT = 0.75f;
    static constexpr float HYPERSPACE_WARP_DURATION = 1.80f;
    static constexpr float HYPERSPACE_JUMP_DIST     = 900.0f;
    static constexpr float BOOST_COOLDOWN_DURATION  = 10.0f;

    int   hyperspaceCharges    = 3;
    int   hyperspaceSpacebar   = 0;
    float hyperspaceSpaceTimer = 0.0f;
    float hyperspaceWarpTimer  = 0.0f;
    bool  hyperspaceJumped     = false;
    float boostCooldownTimer   = 0.0f;
    bool  prevSpace            = false;

    // Research / debug gravitational lensing panel
    LensingDebugSystem lensingDebug;

    GameAudio audio;

    // Settings values mirrored here so they survive menu <-> game transitions
    float mouseSensitivity = 1.0f;  // multiplied into Camera.Sensitivity

    void compile();
    void loadHighScores();
    void saveHighScores();
    void buildSphereMesh(int stacks, int slices);
    void buildDiskMesh();
    void buildPlatformMesh();
    void buildShipMesh();
    void buildTetherGeometry();
    void setupSkyboxGeometry();
    void setupScreenQuad();
    void setupHDRFramebuffer();
    OBJMesh loadOBJMesh(const std::string& path);   // self-contained OBJ parser
    GLuint loadCubemap(std::vector<std::string> faces);
    GLuint loadTexture2D(const std::string& path, bool flip = true);

    void resetGame();
    void spawnPlatformsAndCells();
    void initAsteroids();
    void respawnAsteroid(Asteroid& asteroid, bool randomiseAngle = true);
    void updateAsteroids(float dt);
    void renderAsteroids(const glm::mat4& view, const glm::mat4& proj, float t);
    void updateGame(float dt);
    void updatePickupAnimations(float dt);
    void printGameStatus();
    void updateWindowTitle();
    void drawOverlayUI();
    void drawMainMenu();
    void drawLoadingScreen();

    // Rope simulation
    void initRopeNodes();
    void updateRope(float dt);
    void renderTetherRope(const glm::mat4& view, const glm::mat4& proj);

    // Minigame
    void startTetherMinigame();
    void updateTetherMinigame(float dt);
    void drawTetherMinigameUI();

    void renderCell(const EnergyCell& cell, const glm::mat4& view,
        const glm::mat4& proj, float t);

    void setPlatformUniforms(const glm::mat4& view, const glm::mat4& proj,
        float t);

    void renderShipBeacon(const glm::mat4& view, const glm::mat4& proj, float t);
    void spawnPlatformSetDressings();
    void renderBeaconTowers(const glm::mat4& view, const glm::mat4& proj, float t);
    void renderDebrisProps(const glm::mat4& view, const glm::mat4& proj, float t);
    glm::vec3 getZoneAccentColor(PlatformZoneType type) const;
    float getZoneGlowStrength(PlatformZoneType type) const;
    void renderCockpit(const glm::mat4& view, const glm::mat4& proj, float t);
    void renderCockpitGlass(const glm::mat4& view, const glm::mat4& proj, float t);

    // Shadow map pass
    void setupShadowMap();
    void renderShadowPass();
    void setShadowUniforms();

    // Particle system
    void initParticles();
    void spawnParticle(Particle& p);      // accretion particle — spirals toward black hole
    void spawnDustParticle(Particle& p);  // space dust — slow ambient drift
    void updateParticles(float dt);
    void renderParticles(const glm::mat4& view, const glm::mat4& proj);

public:
    SceneBasic_Uniform();
    void initScene();
    void update(float t);
    void render();
    void resize(int, int);
};

#endif