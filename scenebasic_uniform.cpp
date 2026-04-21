#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

// ─── Constructor ─────────────────────────────────────────────────────────────
SceneBasic_Uniform::SceneBasic_Uniform()
    : fallingIntoBH(false)
    , bhFallDir(0.0f, 0.0f, -1.0f)
    , bhFallT(0.0f)
{}

// ─── compile ─────────────────────────────────────────────────────────────────
void SceneBasic_Uniform::compile()
{
    try {
        prog.compileShader("shader/blackhole.vert");
        prog.compileShader("shader/blackhole.frag");
        prog.link();

        diskProg.compileShader("shader/disk.vert");
        diskProg.compileShader("shader/disk.frag");
        diskProg.link();

        skyboxProg.compileShader("shader/skybox.vert");
        skyboxProg.compileShader("shader/skybox.frag");
        skyboxProg.link();

        hdrProg.compileShader("shader/hdr.vert");
        hdrProg.compileShader("shader/hdr.frag");
        hdrProg.link();

        blurProg.compileShader("shader/hdr.vert");
        blurProg.compileShader("shader/blur.frag");
        blurProg.link();

        platformProg.compileShader("shader/platform.vert");
        platformProg.compileShader("shader/platform.frag");
        platformProg.link();

        // Week 7: Instanced particle system shader
        particleProg.compileShader("shader/particles.vert");
        particleProg.compileShader("shader/particles.frag");
        particleProg.link();

        // Week 8: Shadow map depth pass shader
        shadowProg.compileShader("shader/shadow_depth.vert");
        shadowProg.compileShader("shader/shadow_depth.frag");
        shadowProg.link();

        // Week 6 (CW2): Geometry shader — tether rope ribbon tube
        tetherProg.compileShader("shader/tether.vert");
        tetherProg.compileShader("shader/tether.geom");
        tetherProg.compileShader("shader/tether.frag");
        tetherProg.link();
    }
    catch (GLSLProgramException& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

// ─── resetGame ───────────────────────────────────────────────────────────────
void SceneBasic_Uniform::resetGame()
{
    camera = Camera(glm::vec3(0.0f, 3.5f, 145.0f));
    camera.firstMouse = true;

    audio.stopAllGameSounds();   // clears win/lose endings, boost, void, rumble
    gameWon = false;
    gameLost = false;
    dangerLevel = 0.0f;
    narrativeStage = 0;
    missionFinalTime = 0.0f;
    exposure = 0.85f;
    boostEnergy = 1.0f;
    boostActive = false;
    boostLocked = false;
    boostFlash = 0.0f;
    inCockpit = true;
    shipPosition = camera.Position;
    tetherWarning = 0.0f;
    cockpitPulse = 0.0f;
    glassWarpPhase = 0.0f;
    shipPulsePhase = 0.0f;
    missionStartTime = (float)glfwGetTime();
    playerScore = 0;
    comboCount = 0;
    comboTimer = 0.0f;
    nearDiskSlipstream = 0.0f;
    timeDilationFactor = 1.0f;
    difficultyTier = 0.0f;
    pickupPulse = 0.0f;

    minigameState  = TetherMinigameState::Inactive;
    tether         = TetherRhythm{};
    endStateTimer  = 0.0f;
    fallingIntoBH  = false;
    bhFallT        = 0.0f;
    bhFallDir      = glm::vec3(0.0f, 0.0f, -1.0f);
    // fadeState already reset by caller

    // ── Reset member-variable key state and caches ────────────────────────────
    // Function-local statics cannot be reset, so these are stored as members.
    // Zeroing them here prevents spurious one-frame key-edge detections on the
    // first update() tick after a reset.
    prevX = prevH = prevB = prevF = prevQ = prevEKey = false;
    prevG = prevLMB = prevC = prevTetherA = prevTetherD = false;
    lastHintTime    = -10.0f;
    titleUpdateTimer = 0.0f;
    collectedCells  = 0;

    spawnPlatformsAndCells();
    initAsteroids();
    initRopeNodes();
    lensingDebug.reset();
    // Re-scatter particles on reset
    for (auto& p : particles) spawnParticle(p);

    std::cout << "\n=== BLACK HOLE MISSION RESET ===\n"
        << "Collect all energy cells and return to the platform centre.\n"
        << "Controls: WASD + Mouse | X Toggle EVA/Cockpit\n"
        << "EVA: Press LEFT MOUSE to collect cells\n"
        << "     When tether is stretched, press A/D rhythmically to launch back!\n"
        << "H HDR | B Bloom | F Film | Q/E Exposure | G Reset | ESC Quit\n\n";
}

// ─── printGameStatus ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::printGameStatus()
{
    int collected = collectedCells;

    std::cout << "Cells: " << collected << "/" << energyCells.size()
        << " | Exposure: " << exposure
        << " | Danger: " << dangerLevel
        << " | Film: " << (filmMode ? "ON" : "OFF") << "\n";
}

// ─── updateWindowTitle ───────────────────────────────────────────────────────
void SceneBasic_Uniform::updateWindowTitle()
{
    GLFWwindow* w = glfwGetCurrentContext();
    if (!w) return;

    // Use the cached count — avoids an O(n) loop every title update.
    int collected = collectedCells;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (gameLost)
    {
        oss << "STELLAR EXPEDITION | MISSION FAILED | Cells "
            << collected << "/" << energyCells.size()
            << " | Danger " << dangerLevel;
    }
    else if (gameWon)
    {
        oss << "STELLAR EXPEDITION | MISSION COMPLETE | All cells recovered!";
    }
    else
    {
        oss << "STELLAR EXPEDITION | " << (inCockpit ? "COCKPIT" : "EVA") << " | "
            << "Cells " << collected << "/" << energyCells.size()
            << " | Danger " << dangerLevel
            << " | Boost " << boostEnergy
            << " | Tether " << glm::length(camera.Position - shipPosition);
    }

    glfwSetWindowTitle(w, oss.str().c_str());
}

// ─── updatePickupAnimations ──────────────────────────────────────────────────
void SceneBasic_Uniform::updatePickupAnimations(float dt)
{
    for (auto& cell : energyCells)
    {
        if (cell.collected || cell.phase == PickupPhase::Idle) continue;

        cell.phaseTimer += dt;

        if (cell.phase == PickupPhase::SpinShrink)
        {
            float p = glm::clamp(cell.phaseTimer / EnergyCell::SPIN_SHRINK_DURATION, 0.f, 1.f);

            cell.spinAngle += dt * (4.0f + p * p * 40.0f);
            cell.cellScale = 1.0f - glm::smoothstep(0.6f, 1.0f, p);
            cell.poofAlpha = 1.0f;
            cell.orbScale = glm::smoothstep(0.2f, 1.0f, p) * 0.5f;

            if (p >= 1.0f) {
                cell.phase = PickupPhase::Poof;
                cell.phaseTimer = 0.0f;
                cell.cellScale = 0.0f;
                cell.orbScale = 0.5f;
                cell.poofAlpha = 1.0f;
                shakeDuration = 0.45f;
                shakeMagnitude = 0.18f;
            }
        }
        else if (cell.phase == PickupPhase::Poof)
        {
            float p = glm::clamp(cell.phaseTimer / EnergyCell::POOF_DURATION, 0.f, 1.f);
            cell.orbScale = 0.5f + p * 0.5f;
            cell.poofAlpha = 1.0f - (p * p);

            if (p >= 1.0f)
            {
                cell.collected = true;
                collectedCells++;   // keep cached count in sync
            }
        }
    }
}

// ─── updateGame ──────────────────────────────────────────────────────────────
void SceneBasic_Uniform::updateGame(float dt)
{
    if (gameWon || gameLost) return;

    float nowTime = (float)glfwGetTime();
    float missionElapsed = glm::max(0.0f, nowTime - missionStartTime);

    // collectedCells is maintained by updatePickupAnimations() — no loop needed.
    float progress01 = energyCells.empty() ? 0.0f
        : static_cast<float>(collectedCells) / static_cast<float>(energyCells.size());

    // Difficulty ramps up as the run goes on and as more cells are collected.
    difficultyTier = glm::clamp(progress01 * 0.70f + glm::smoothstep(25.0f, 120.0f, missionElapsed) * 0.30f, 0.0f, 1.0f);

    glm::vec3 toBH = bhPos - camera.Position;
    float distToBH = glm::length(toBH);
    glm::vec3 bhDir = glm::normalize(toBH);

    const glm::vec3 mainPlatformPos(0.0f, 1.0f, 145.0f);

    const float safeRadius = 9.0f;
    float closestPlatDist = glm::length(camera.Position - mainPlatformPos);
    for (const auto& zone : platformZones)
        closestPlatDist = glm::min(closestPlatDist, glm::length(camera.Position - zone.position));

    float platInfluence = glm::clamp((closestPlatDist - safeRadius) / safeRadius, 0.f, 1.f);

    float gravityStartR = bhR + 1500.0f;
    float lethalR = bhR + 180.0f;

    float normDist = glm::clamp((distToBH - lethalR) / (gravityStartR - lethalR), 0.f, 1.f);
    float proximity = 1.0f - normDist;
    proximity = pow(proximity, 1.35f);

    float diskHeight = fabsf(camera.Position.y - bhPos.y);
    nearDiskSlipstream = (1.0f - glm::smoothstep(75.0f, 260.0f, diskHeight)) *
        (1.0f - glm::smoothstep(420.0f, 1450.0f, distToBH));
    nearDiskSlipstream = glm::clamp(nearDiskSlipstream, 0.0f, 1.0f);

    // Time dilation: near the BH the displayed timer runs slow while real time
    // (and score accumulation) continues normally. Deep proximity = 40% slowdown.
    timeDilationFactor = glm::mix(1.0f, 0.38f, proximity * 0.82f);

    float platformShield = glm::mix(0.28f, 1.0f, platInfluence);
    dangerLevel = glm::clamp(proximity * glm::mix(0.45f, 1.0f, platInfluence), 0.f, 1.f);

    float pullSpeed = (1.8f + proximity * proximity * (42.0f + 10.0f * difficultyTier)) * platformShield;
    float redZoneBoost = 1.0f + glm::smoothstep(0.78f, 1.0f, dangerLevel) * 10.0f;
    pullSpeed *= redZoneBoost;

    // Danger alarm sound: fire once per 4 s when in the red zone
    static float alarmCooldown = 0.0f;
    alarmCooldown -= dt;
    if (dangerLevel > 0.78f && alarmCooldown <= 0.0f) {
        audio.playDangerAlarm();
        alarmCooldown = 4.0f - dangerLevel * 2.0f;
    }

    // ── HELIO ARRAY COLLAPSE — narrative transmissions ────────────────────────
    static float narrativeCooldown = 0.0f;
    narrativeCooldown -= dt;
    struct NarrLine { float threshold; int stage; const char* line; };
    static const NarrLine narr[] = {
        { 0.05f, 0, "[HELIO ARRAY] Responding... partially." },
        { 0.10f, 1, "[ARRAY LOG] Energy signatures detected." },
        { 0.30f, 2, "[WARNING] Energy flow reversed." },
        { 0.45f, 3, "[ARRAY LOG] The array is feeding the singularity." },
        { 0.60f, 4, "[CRITICAL] Gravitational spike detected." },
        { 0.72f, 5, "[ARRAY LOG] This is not a system failure." },
        { 0.82f, 6, "[ARRAY LOG] ...It's intentional." },
    };
    static const int NARR_COUNT = 7;
    for (int ni = 0; ni < NARR_COUNT; ni++) {
        if (narrativeStage == narr[ni].stage
            && dangerLevel >= narr[ni].threshold
            && narrativeCooldown <= 0.0f) {
            std::cout << narr[ni].line << "\n";
            narrativeStage++;
            narrativeCooldown = 5.0f;
            break;
        }
    }

    camera.Position += bhDir * pullSpeed * dt;

    // Slipstream velocity assist: push the player along the disk tangent
    if (nearDiskSlipstream > 0.25f && inCockpit && !gameWon && !gameLost)
    {
        // Tangent = cross(up, toward-BH) gives a circling push around the disk
        glm::vec3 diskTangent = glm::normalize(glm::cross(glm::vec3(0, 1, 0), bhDir));
        float assistStrength = (nearDiskSlipstream - 0.25f) / 0.75f;  // 0 at 25%, 1 at 100%
        camera.Position += diskTangent * assistStrength * 8.5f * dt;
    }

    const float collectRange = 8.0f;
    // lastHintTime is a member variable (not a static) so resetGame() can clear it.

    for (const auto& cell : energyCells)
    {
        if (cell.phase != PickupPhase::Idle) continue;

        if (glm::length(camera.Position - cell.position) < collectRange)
        {
            if (nowTime - lastHintTime > 3.0f)
            {
                std::cout << "[Left Mouse] Click to collect energy cell.";
                lastHintTime = nowTime;
            }
            break;
        }
    }

    updatePickupAnimations(dt);

    // Use the cached count — no per-frame loop required.
    bool allDone = !energyCells.empty()
        && collectedCells >= static_cast<int>(energyCells.size());

    comboTimer = glm::max(0.0f, comboTimer - dt);
    if (comboTimer <= 0.0f)
        comboCount = 0;
    pickupPulse = glm::max(0.0f, pickupPulse - dt * 2.5f);

    if (distToBH < lethalR) {
        gameLost       = true;
        fallingIntoBH  = true;
        bhFallDir      = glm::normalize(bhPos - camera.Position);
        fadeState      = 6;
        endStateTimer  = 0.0f;
        audio.startBHFall();
        audio.playLoseEnding();
        std::cout << "*** MISSION FAILED: Consumed by the black hole. ***\n";
    }

    // Win: all cells + return to main platform (widened radius 8.0 for comfort).
    // HOME_PLATFORM_X/WIN_Y/Z are shared constants — changing them here also
    // updates the HUD nav marker and shadow pass automatically.
    if (allDone
        && glm::length(camera.Position
            - glm::vec3(HOME_PLATFORM_X, HOME_WIN_Y, HOME_PLATFORM_Z)) < 8.0f
        && !gameWon)
    {
        gameWon = true;
        fadeState = 4;
        endStateTimer = 0.0f;
        missionFinalTime = missionElapsed;
        audio.playWinFanfare();
        audio.playWinEnding();
        int timeBonus = glm::max(0, 300 - (int)missionElapsed) * 5;
        playerScore += 500 + timeBonus;
        inCockpit = true;
        // Record score in leaderboard
        highScores.push_back({ playerScore, missionFinalTime });
        std::sort(highScores.begin(), highScores.end(),
            [](const std::pair<int,float>& a, const std::pair<int,float>& b){ return a.first > b.first; });
        if (highScores.size() > 10) highScores.resize(10);
        saveHighScores();
        std::cout << "*** MISSION COMPLETE: All energy cells recovered! ***\n";
    }
}

// update
void SceneBasic_Uniform::update(float t)
{
    static float lastT = 0.0f;
    float dt = (lastT == 0.0f) ? 0.0f : t - lastT;
    dt = glm::clamp(dt, 0.0f, 0.1f);
    lastT = t;

    GLFWwindow* w = glfwGetCurrentContext();

    menuTime += dt;
    menuBHAngle += dt * 0.22f;

    // ── Loading screen update ─────────────────────────────────────────────────
    if (showLoading)
    {
        loadingProgress += dt / 2.8f;  // 2.8s fake loading
        if (loadingProgress >= 1.0f)
        {
            loadingProgress = 1.0f;
            showLoading = false;
            showMenu    = false;
            audio.stopMenuMusic();
            resetGame();
            camera.Sensitivity = 0.1f * mouseSensitivity;
            GLFWwindow* w2 = glfwGetCurrentContext();
            glfwSetInputMode(w2, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        updateWindowTitle();
        return;
    }

    if (showMenu)
    {
        static bool upPrev = false, downPrev = false, enterPrev = false;
        static bool escPrev2 = false, leftPrev = false, rightPrev = false;
        bool upNow    = glfwGetKey(w, GLFW_KEY_UP)    == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
        bool downNow  = glfwGetKey(w, GLFW_KEY_DOWN)  == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
        bool leftNow  = glfwGetKey(w, GLFW_KEY_LEFT)  == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        bool rightNow = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        bool enterNow = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool escNow2  = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        if (menuPage == 0)
        {
            // Main menu navigation — 4 items: PLAY, LEADERBOARD, SETTINGS, EXIT
            if (upNow   && !upPrev)   menuSelection = (menuSelection + 3) % 4;
            if (downNow && !downPrev) menuSelection = (menuSelection + 1) % 4;
            if (enterNow && !enterPrev)
            {
                if      (menuSelection == 0) { showLoading = true; loadingProgress = 0.0f; }
                else if (menuSelection == 1) { menuPage = 1; }  // leaderboard
                else if (menuSelection == 2) { menuPage = 2; settingsSelection = 0; }  // settings
                else if (menuSelection == 3) glfwSetWindowShouldClose(w, true);
            }
            if (escNow2 && !escPrev2) glfwSetWindowShouldClose(w, true);
        }
        else if (menuPage == 1)
        {
            // Leaderboard — any key returns
            if ((enterNow && !enterPrev) || (escNow2 && !escPrev2)) menuPage = 0;
        }
        else if (menuPage == 2)
        {
            // Settings page — 5 rows: Music Vol, SFX Vol, Sensitivity, Controls, Back
            static const int NUM_SETTINGS = 5;
            if (upNow   && !upPrev)   settingsSelection = (settingsSelection + NUM_SETTINGS - 1) % NUM_SETTINGS;
            if (downNow && !downPrev) settingsSelection = (settingsSelection + 1) % NUM_SETTINGS;

            const float STEP = 0.05f;
            if (settingsSelection == 0) {
                if (leftNow  && !leftPrev)  audio.musicVol = glm::clamp(audio.musicVol - STEP, 0.0f, 1.0f);
                if (rightNow && !rightPrev) audio.musicVol = glm::clamp(audio.musicVol + STEP, 0.0f, 1.0f);
                audio.applyVolumes();
            }
            else if (settingsSelection == 1) {
                if (leftNow  && !leftPrev)  audio.sfxVol = glm::clamp(audio.sfxVol - STEP, 0.0f, 1.0f);
                if (rightNow && !rightPrev) audio.sfxVol = glm::clamp(audio.sfxVol + STEP, 0.0f, 1.0f);
            }
            else if (settingsSelection == 2) {
                if (leftNow  && !leftPrev)  mouseSensitivity = glm::clamp(mouseSensitivity - 0.1f, 0.2f, 3.0f);
                if (rightNow && !rightPrev) mouseSensitivity = glm::clamp(mouseSensitivity + 0.1f, 0.2f, 3.0f);
                camera.Sensitivity = 0.1f * mouseSensitivity;
            }
            // row 3 = Controls (display only), row 4 = Back
            if (enterNow && !enterPrev) {
                if (settingsSelection == 4) menuPage = 0;
            }
            if (escNow2 && !escPrev2) menuPage = 0;
        }

        upPrev = upNow; downPrev = downNow; leftPrev = leftNow; rightPrev = rightNow;
        enterPrev = enterNow; escPrev2 = escNow2;
        updateWindowTitle();
        return;
    }

    glassWarpPhase += dt * 0.8f;
    shipPulsePhase += dt * 1.4f;

    lensingDebug.handleInput(w);
    lensingDebug.update(dt);

    // prevX is a member variable — see header.  No static needed.

    if (fadeState == 0 || fadeState == 3)
    {
        bool shiftHeld =
            glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool xNow = glfwGetKey(w, GLFW_KEY_X) == GLFW_PRESS;

        if (xNow && !prevX)
        {
            if (inCockpit)
            {
                inCockpit = false;
                shipPosition = camera.Position;
                camera.Position += camera.Front * 6.5f - camera.Up * 0.25f;
                boostActive = false;
                initRopeNodes();
                minigameState = TetherMinigameState::Inactive;
                tetherLockedDist = 0.0f;
                std::cout << "EVA engaged. Tether active.\n";
            }
            else
            {
                float reentryDist = glm::length(camera.Position - shipPosition);
                if (reentryDist <= shipReentryDistance)
                {
                    inCockpit = true;
                    camera.Position = shipPosition;
                    boostEnergy = glm::min(1.0f, boostEnergy + 0.15f);
                    minigameState = TetherMinigameState::Inactive;
                    tetherLockedDist = 0.0f;
                    std::cout << "Re-entered cockpit.\n";
                }
                else
                {
                    std::cout << "Move closer to the ship to re-enter (or use A/D minigame!).\n";
                }
            }
        }
        prevX = xNow;

        boostActive = false;
        float moveMultiplier = inCockpit ? 1.0f : evaMoveMultiplier;

        if (inCockpit)
        {
            // Clear lockout once energy recovers to 25%
            if (boostLocked && boostEnergy >= 0.25f)
                boostLocked = false;

            if (shiftHeld && boostEnergy > 0.001f && !boostLocked && !gameWon && !gameLost)
            {
                audio.startBoost();
                boostActive = true;
                moveMultiplier = boostMultiplier;
                boostEnergy = glm::max(0.0f, boostEnergy - boostDrainRate * dt);

                if (boostEnergy <= 0.0f)
                {
                    boostLocked = true;   // deplete → lock until 25% recharged
                    boostFlash = 0.25f;
                }
                else if (boostEnergy <= 0.05f)
                    boostFlash = 0.25f;
            }
            else
            {
                audio.stopBoost();
                float slipMul = glm::mix(1.0f, 3.20f, nearDiskSlipstream);
                boostEnergy = glm::min(1.0f,
                    boostEnergy + boostRechargeRate * dt * 0.80f * slipMul);
            }
        }
        else
        {
            // Smaller slipstream recharge bonus during EVA
            float slipMul = glm::mix(0.85f, 1.80f, nearDiskSlipstream);
            boostEnergy = glm::min(1.0f,
                boostEnergy + boostRechargeRate * dt * 0.55f * slipMul);
        }

        // Minigame blocks normal movement keys (A/D consumed by minigame)
        if (minigameState != TetherMinigameState::Active)
        {
            camera.processKeyboard(w, moveMultiplier);
        }
        camera.processMouse(w);

        if (inCockpit)
        {
            shipPosition = camera.Position;
        }
        else
        {
            float tetherDist = glm::length(camera.Position - shipPosition);
            float tetherT = glm::clamp(
                (tetherDist - tetherMaxDistance * 0.65f) / (tetherMaxDistance * 0.35f),
                0.0f, 1.0f
            );
            tetherWarning = tetherT;

            if (minigameState == TetherMinigameState::Active)
            {
                // While minigame is active, enforce the locked distance as ceiling.
                // This stops BH gravity and tether stretch from dragging the player
                // back out after a successful reel-in press.
                if (tetherDist > tetherLockedDist && tetherLockedDist > 0.5f)
                {
                    glm::vec3 dirBack = glm::normalize(shipPosition - camera.Position);
                    camera.Position = shipPosition - dirBack * tetherLockedDist;
                }
            }
            else
            {
                // Normal tether hard limit
                if (tetherDist > tetherMaxDistance)
                {
                    glm::vec3 dirBack = glm::normalize(shipPosition - camera.Position);
                    camera.Position = shipPosition - dirBack * tetherMaxDistance;
                    boostFlash = 0.2f;

                    if (minigameState == TetherMinigameState::Inactive)
                    {
                        tetherLockedDist = tetherMaxDistance;  // start lock at full extent
                        startTetherMinigame();
                    }
                }
            }

            updateTetherMinigame(dt);

            // Success: smoothly reel remaining distance in
            if (minigameState == TetherMinigameState::Success)
            {
                float dist = glm::length(camera.Position - shipPosition);
                if (dist > 0.5f)
                {
                    glm::vec3 dir = glm::normalize(shipPosition - camera.Position);
                    camera.Position += dir * 28.0f * dt;
                    // Keep locked dist in sync so clamp doesn't fight the reel
                    tetherLockedDist = glm::length(camera.Position - shipPosition);
                }
                else
                {
                    minigameState = TetherMinigameState::Inactive;
                    tetherLockedDist = 0.0f;
                }
            }

            updateRope(dt);
        }
    }

    lightOrbitAngle += 0.35f * dt;
    lightPos = glm::vec3(
        sinf(lightOrbitAngle) * 18.0f,
        6.0f + sinf(lightOrbitAngle * 0.5f) * 4.0f,
        -60.0f + cosf(lightOrbitAngle) * 18.0f
    );

    // All prev* booleans are member variables (see header) so they are cleared
    // by resetGame() and cannot carry stale state across mission restarts.

    bool hNow = glfwGetKey(w, GLFW_KEY_H) == GLFW_PRESS;
    if (hNow && !prevH) { hdrEnabled = !hdrEnabled; std::cout << "HDR: " << (hdrEnabled ? "ON" : "OFF") << "\n"; }
    prevH = hNow;

    bool bNow = glfwGetKey(w, GLFW_KEY_B) == GLFW_PRESS;
    if (bNow && !prevB) { bloomEnabled = !bloomEnabled; std::cout << "Bloom: " << (bloomEnabled ? "ON" : "OFF") << "\n"; }
    prevB = bNow;

    bool fNow = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
    if (fNow && !prevF) { filmMode = !filmMode; std::cout << "Film Mode: " << (filmMode ? "ON" : "OFF") << "\n"; }
    prevF = fNow;

    bool nNow = glfwGetKey(w, GLFW_KEY_N) == GLFW_PRESS;
    if (nNow && !prevN) {
        nightVisionMode = !nightVisionMode;
        std::cout << "Night Vision: " << (nightVisionMode ? "ON" : "OFF") << "\n";
    }
    prevN = nNow;

    bool qNow = glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS;
    if (qNow && !prevQ) { exposure = glm::max(0.15f, exposure - 0.25f); std::cout << "Exposure: " << exposure << "\n"; }
    prevQ = qNow;

    bool eNow = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
    if (eNow && !prevEKey) { exposure = glm::min(5.0f, exposure + 0.25f); std::cout << "Exposure: " << exposure << "\n"; }
    prevEKey = eNow;

    bool lmbNow = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (!inCockpit && lmbNow && !prevLMB && !gameWon && !gameLost)
    {
        const float collectRange = 8.0f;
        EnergyCell* nearest = nullptr;
        float nearestDist = collectRange;

        for (auto& cell : energyCells)
        {
            if (cell.phase != PickupPhase::Idle) continue;
            float d = glm::length(camera.Position - cell.position);
            if (d < nearestDist) {
                nearestDist = d;
                nearest = &cell;
            }
        }

        if (nearest)
        {
            nearest->phase = PickupPhase::SpinShrink;
            nearest->phaseTimer = 0.0f;
            audio.playCellCollect();
            std::cout << "Energy cell collected in EVA! ("
                << (collectedCells + 1) << "/" << energyCells.size() << ")\n";
            printGameStatus();
        }
    }
    prevLMB = lmbNow;

    bool gNow = glfwGetKey(w, GLFW_KEY_G) == GLFW_PRESS;
    if (gNow && !prevG && (fadeState == 0 || fadeState == 5 || fadeState == 7)) {
        fadeAlpha = 0.0f; fadeState = 0; endStateTimer = 0.0f;
        fallingIntoBH = false; bhFallT = 0.0f;
        dangerLevel = 0.0f;    // reset so rumble silences immediately
        audio.stopAllGameSounds();
        audio.playMenuMusic();
        menuPage = 0;
        showMenu = true;
        glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    prevG = gNow;

    // C — instant collect all remaining energy cells (debug / testing shortcut)
    bool cNow = glfwGetKey(w, GLFW_KEY_C) == GLFW_PRESS;
    if (cNow && !prevC && !gameWon && !gameLost)
    {
        int grabbed = 0;
        for (auto& cell : energyCells)
        {
            if (!cell.collected && cell.phase == PickupPhase::Idle)
            {
                cell.phase = PickupPhase::SpinShrink;
                cell.phaseTimer = 0.0f;
                grabbed++;
            }
        }
        if (grabbed > 0)
            std::cout << "[DEBUG] Instant-collected " << grabbed << " cell(s).\n";
    }
    prevC = cNow;

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    if (shakeDuration > 0.0f)
        shakeDuration = glm::max(0.0f, shakeDuration - dt);

    if (boostFlash > 0.0f)
        boostFlash = glm::max(0.0f, boostFlash - dt);

    cockpitPulse += dt;
    if (inCockpit)
        tetherWarning = glm::max(0.0f, tetherWarning - dt * 1.5f);

    updateAsteroids(dt);
    updateParticles(dt);  // Week 7: advance particle ages
    updateGame(dt);

    // ── IrrKlang per-frame audio update ─────────────────────────────────────
    audio.update(dangerLevel, fallingIntoBH);

    // fadeState 1/2/3 no longer used (replaced by 6/7 BH-fall cinematic)

    if (fadeState == 6)
    {
        // ── 22-second Interstellar BH-fall cinematic ───────────────────────────
        //  0– 6s   camera accelerates into BH; red-shift builds
        //  4– 7s   fade to black (event horizon breach)
        //  7–14s   tesseract phase: infinite orange data-grid
        // 14–20s   echoed mission memories, singularity message
        // 20–22s   fade to black → static lost screen
        endStateTimer += dt;

        // Animate camera spiralling into BH during approach phase
        if (endStateTimer < 6.5f)
        {
            float fallT  = glm::clamp(endStateTimer / 6.5f, 0.0f, 1.0f);
            float accel  = 55.0f + fallT * fallT * 2200.0f;
            camera.Position += bhFallDir * accel * dt;
            // Slow spiral: rotate bhFallDir slightly each frame
            glm::vec3 upVec  = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 right2 = bhFallDir - upVec * glm::dot(bhFallDir, upVec);
            if (glm::length(right2) > 0.001f)
                right2 = glm::normalize(right2);
            bhFallDir = glm::normalize(bhFallDir + right2 * 0.008f * (1.0f - fallT));
        }

        bhFallT = glm::clamp(endStateTimer / 22.0f, 0.0f, 1.0f);

        // Trigger audio cues at specific times (use static bools to fire once each)
        static bool firedHorizon = false, firedVoid = false;
        if (endStateTimer < 0.1f) { firedHorizon = false; firedVoid = false; }
        if (!firedHorizon && endStateTimer >= 5.0f) {
            audio.playEventHorizon();
            firedHorizon = true;
        }
        if (!firedVoid && endStateTimer >= 8.0f) {
            audio.startVoidAmbient();
            firedVoid = true;
        }

        // Fade-to-black envelope: transparent until 4 s, opaque 7–20 s, fade out at 20 s
        if (endStateTimer < 4.0f)
            fadeAlpha = 0.0f;
        else if (endStateTimer < 7.0f)
            fadeAlpha = glm::clamp((endStateTimer - 4.0f) / 3.0f, 0.0f, 1.0f);
        else if (endStateTimer < 20.0f)
            fadeAlpha = 1.0f;
        else if (endStateTimer < 22.0f)
            fadeAlpha = glm::max(0.0f, 1.0f - (endStateTimer - 20.0f) / 2.0f);
        else
        {
            fadeAlpha  = 0.0f;
            fadeState  = 7;
        }
    }
    else if (fadeState == 7)
    {
        // Static BH lost screen — waits for G
        fadeAlpha = 0.0f;
    }
    else if (fadeState == 4)
    {
        // ── 18-second win cutscene ────────────────────────────────────────────
        //  0.0–2.5s  streaks (pure black bg, exposure ramps)
        //  2.5–4.5s  fade to white + hold
        //  4.5–5.2s  fade back to dark space
        //  5.2–12s   four planet flybys
        //  12–18s    Earth grows in, win panel fades over it
        endStateTimer += dt;
        const float WHITE_START = 2.5f;
        const float WHITE_HOLD = 4.5f;
        const float FADE_BACK = 5.2f;
        const float SCENE_END = 18.0f;

        if (endStateTimer < WHITE_START)
            fadeAlpha = 0.0f;
        else if (endStateTimer < WHITE_HOLD)
            fadeAlpha = glm::clamp((endStateTimer - WHITE_START) / (WHITE_HOLD - WHITE_START), 0.0f, 1.0f);
        else if (endStateTimer < FADE_BACK)
            fadeAlpha = glm::max(0.0f, 1.0f - (endStateTimer - WHITE_HOLD) / (FADE_BACK - WHITE_HOLD));
        else if (endStateTimer < SCENE_END)
            fadeAlpha = 0.0f;
        else
        {
            fadeAlpha = 0.0f;
            fadeState = 5;
        }
    }
    else if (fadeState == 5)
    {
        // Win screen — just sit here until G is pressed
        fadeAlpha = 0.0f;
    }

    // Throttle glfwSetWindowTitle() to ~4 Hz.  The title contains diagnostic
    // floats that change every frame, so rebuilding the string + issuing a
    // driver call 60+ times per second is wasteful.
    titleUpdateTimer -= dt;
    if (titleUpdateTimer <= 0.0f)
    {
        titleUpdateTimer = 0.25f;
        updateWindowTitle();
    }
}

void SceneBasic_Uniform::initScene()
{
    compile();
    audio.init();
    audio.playMenuMusic();  // ambient menu music starts immediately

    loadHighScores();

    // Seed RNG for varied asteroid/debris layout each session
    srand(static_cast<unsigned int>(time(nullptr)));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    buildSphereMesh(128, 128);
    buildDiskMesh();
    buildPlatformMesh();
    buildShipMesh();
    buildTetherGeometry();
    setupSkyboxGeometry();
    setupScreenQuad();
    setupHDRFramebuffer();
    initParticles();
    setupShadowMap();

    // Load Blender-exported cell mesh (no Assimp — pure std::ifstream OBJ parser).
    // Falls back to the procedural sphere silently if the file is missing.
    cellMesh = loadOBJMesh("media/cell.obj");
    if (!cellMesh.loaded)
        std::cout << "[Init] media/cell.obj not found — energy cells will use procedural sphere.\n";

    skyboxTex = loadCubemap({
        "media/skybox/right.png",  "media/skybox/left.png",
        "media/skybox/top.png",    "media/skybox/bottom.png",
        "media/skybox/front.png",  "media/skybox/back.png"
        });

    satellitePlatformTex = loadTexture2D("media/blue_metal_plate_diff_4k.jpg", true);

    if (!satellitePlatformTex)
        std::cout << "[Init] blue_metal_plate_diff_4k.jpg not found — using flat colour.\n";

    // Menu starts with visible cursor; game hides it
    GLFWwindow* win = glfwGetCurrentContext();
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glViewport(0, 0, width, height);
    // Do NOT call resetGame() here — the menu is showing
}

// ─── render ──────────────────────────────────────────────────────────────────
void SceneBasic_Uniform::render()
{
    float t = (float)glfwGetTime();

    if (showLoading || showMenu) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST); glEnable(GL_SCISSOR_TEST);
        if (showLoading) drawLoadingScreen();
        else             drawMainMenu();
        glDisable(GL_SCISSOR_TEST); glEnable(GL_DEPTH_TEST);
        return;
    }

    glm::mat4 view = camera.getViewMatrix();

    // ── Week 8: Shadow pass (depth map from light POV) ────────────────────────
    renderShadowPass();

    // Bind the shadow map and set its uniforms once for the entire frame.
    // Previously setShadowUniforms() was called inside setPlatformUniforms(),
    // which is invoked separately for platforms, asteroids, cells, beacons,
    // debris, and the ship — causing 6+ redundant texture binds per frame.
    setShadowUniforms();

    if (shakeDuration > 0.0f)
    {
        float strength = shakeMagnitude * (shakeDuration / 0.45f);
        float shakeX = sinf(t * 97.0f) * strength;
        float shakeY = sinf(t * 73.0f) * strength;
        view = glm::translate(view, glm::vec3(shakeX, shakeY, 0.0f));
    }

    float fov = inCockpit ? 78.0f : 60.0f;
    float nearPlane = inCockpit ? 0.02f : 0.10f;
    glm::mat4 proj = glm::perspective(glm::radians(fov),
        (float)width / (float)height,
        nearPlane, 5000.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ── Skybox ────────────────────────────────────────────────────────────────
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    skyboxProg.use();
    skyboxProg.setUniform("View", glm::mat4(glm::mat3(view)));
    skyboxProg.setUniform("Projection", proj);
    skyboxProg.setUniform("skybox", 0);
    skyboxProg.setUniform("uTime", t);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // ── Platforms ─────────────────────────────────────────────────────────────
    setPlatformUniforms(view, proj, t);

    platformProg.use();
    platformProg.setUniform("uBaseColor", glm::vec3(0.04f, 0.05f, 0.07f));
    platformProg.setUniform("uGlowColor", glm::vec3(0.18f, 0.85f, 1.20f));
    platformProg.setUniform("uGlowStrength", 1.8f);
    platformProg.setUniform("uIsCore", 0);
    platformProg.setUniform("uHasDiffuse", satellitePlatformTex ? 1 : 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uLightPos2", glm::vec3(0.0f, 4.0f, 145.0f));
    platformProg.setUniform("uLightColor2", glm::vec3(0.18f, 0.85f, 1.20f));
    platformProg.setUniform("uLightStrength2", 0.6f);

    glm::mat4 upperModel =
        glm::translate(glm::mat4(1.f), glm::vec3(0, 0.8f, 145.0f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(1.9f, 1.f, 1.9f));
    platformProg.setUniform("Model", upperModel);

    glBindVertexArray(platVAO);
    glDrawArrays(GL_TRIANGLES, 0, platVertCount);

    platformProg.setUniform("uBaseColor", glm::vec3(0.03f, 0.04f, 0.06f));
    platformProg.setUniform("uGlowColor", glm::vec3(0.18f, 0.85f, 1.20f));
    platformProg.setUniform("uGlowStrength", 1.4f);

    glm::mat4 lowerModel =
        glm::translate(glm::mat4(1.f), glm::vec3(0, -1.1f, 145.0f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(2.35f, 1.f, 2.35f));
    platformProg.setUniform("Model", lowerModel);
    glDrawArrays(GL_TRIANGLES, 0, platVertCount);
    glBindVertexArray(0);

    for (size_t i = 0; i < platformZones.size(); i++)
    {
        const PlatformZone& zone = platformZones[i];
        glm::vec3 cellLightPos = zone.position + glm::vec3(0.0f, 2.5f, 0.0f);

        glm::vec3 baseColor(0.03f, 0.04f, 0.06f);  // dark gunmetal
        glm::vec3 glowColor = zone.accentColor;
        float glowStrength = zone.glowStrength * 1.6f;  // stronger neon rim

        // Week 10 PBR: per-zone material — condition affects surface roughness.
        // HomeRelay: well-maintained, semi-polished plating → lower roughness.
        // StableRelay: standard operational metal → moderate roughness.
        // DamagedRelay: corroded/pitted surface → non-metallic, high roughness.
        // HazardRelay: heavily degraded, near-crumbling → near-dielectric roughness.
        float zoneMetallic  = 0.15f;
        float zoneRoughness = 0.55f;
        if (zone.type == PlatformZoneType::DamagedRelay) {
            baseColor     = glm::vec3(0.05f, 0.04f, 0.05f);
            zoneMetallic  = 0.06f;   // corrosion strips metallic sheen
            zoneRoughness = 0.82f;   // heavily pitted
        } else if (zone.type == PlatformZoneType::HazardRelay) {
            baseColor     = glm::vec3(0.06f, 0.04f, 0.03f);
            zoneMetallic  = 0.03f;   // oxidised beyond recognition
            zoneRoughness = 0.94f;
        } else if (zone.type == PlatformZoneType::HomeRelay) {
            baseColor     = glm::vec3(0.03f, 0.05f, 0.08f);
            zoneMetallic  = 0.22f;   // freshly maintained, clean metal
            zoneRoughness = 0.42f;   // polished surface → tighter specular
        }

        platformProg.use();
        platformProg.setUniform("uBaseColor", baseColor);
        platformProg.setUniform("uGlowColor", glowColor);
        platformProg.setUniform("uGlowStrength", glowStrength);
        platformProg.setUniform("uIsCore", 0);
        platformProg.setUniform("uHasDiffuse", satellitePlatformTex ? 1 : 0);
        platformProg.setUniform("uHasNormal", 0);
        platformProg.setUniform("uMetallic",  zoneMetallic);
        platformProg.setUniform("uRoughness", zoneRoughness);
        platformProg.setUniform("uLightPos2", cellLightPos);
        platformProg.setUniform("uLightColor2", glowColor * 1.2f);
        platformProg.setUniform("uLightStrength2", 1.1f);

        if (satellitePlatformTex)
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, satellitePlatformTex);
        }

        glm::mat4 satModel =
            glm::translate(glm::mat4(1.f), zone.position) *
            glm::scale(glm::mat4(1.f), glm::vec3(0.75f, 1.f, 0.75f));
        platformProg.setUniform("Model", satModel);

        glBindVertexArray(platVAO);
        glDrawArrays(GL_TRIANGLES, 0, platVertCount);
        glBindVertexArray(0);
    }

    renderBeaconTowers(view, proj, t);
    renderDebrisProps(view, proj, t);

    for (const auto& cell : energyCells) {
        if (!cell.collected)
            renderCell(cell, view, proj, t);
    }

    renderAsteroids(view, proj, t);
    renderShipBeacon(view, proj, t);

    // ── Space-dust particles (Week 7: instanced particle system) ──────────────
    renderParticles(view, proj);

    // ── Tether rope (in EVA) — blend state managed inside renderTetherRope ─────
    if (!inCockpit)
        renderTetherRope(view, proj);

    // ── Accretion disk (back) ─────────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    diskProg.use();
    glm::mat4 diskModel =
        glm::translate(glm::mat4(1.f), bhPos) *
        glm::rotate(glm::mat4(1.f), glm::radians(12.f), glm::vec3(1, 0, 0)) *
        glm::scale(glm::mat4(1.f), glm::vec3(bhR * 1.45f));

    diskProg.setUniform("Model", diskModel);
    diskProg.setUniform("View", view);
    diskProg.setUniform("Projection", proj);
    diskProg.setUniform("uTime", t);
    diskProg.setUniform("uLightPos", lightPos);
    diskProg.setUniform("uCamPos", camera.Position);
    diskProg.setUniform("uBHPos", bhPos);
    diskProg.setUniform("uDrawBack", 1);

    glBindVertexArray(diskVAO);
    glDrawArrays(GL_TRIANGLES, 0, diskVertCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ── Black hole sphere ─────────────────────────────────────────────────────
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    prog.use();
    glm::mat4 sphereModel =
        glm::translate(glm::mat4(1.f), bhPos) *
        glm::scale(glm::mat4(1.f), glm::vec3(bhR));

    prog.setUniform("Model", sphereModel);
    prog.setUniform("View", view);
    prog.setUniform("Projection", proj);
    prog.setUniform("uTime", t);
    prog.setUniform("uSkybox", 0);
    prog.setUniform("uBHPos", bhPos);
    prog.setUniform("uCamPos", camera.Position);
    prog.setUniform("uBHScale", bhR / 3.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glDisable(GL_CULL_FACE);

    // ── Accretion disk (front) ────────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    diskProg.use();
    diskProg.setUniform("uBHPos", bhPos);
    diskProg.setUniform("uCamPos", camera.Position);
    diskProg.setUniform("uDrawBack", 0);
    // front pass reuses uniforms already sent to back pass

    glBindVertexArray(diskVAO);
    glDrawArrays(GL_TRIANGLES, 0, diskVertCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ── Cockpit interior + glass ───────────────────────────────────────────────
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderCockpit(view, proj, t);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // ── Bloom blur passes ─────────────────────────────────────────────────────
    bool horizontal = true, firstPass = true;

    blurProg.use();
    blurProg.setUniform("image", 0);
    blurProg.setUniform("uResolution", glm::vec2((float)width, (float)height));

    for (int i = 0; i < 10; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal ? 1 : 0]);
        blurProg.setUniform("horizontal", horizontal ? 1 : 0);
        blurProg.setUniform("firstPass", firstPass ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,
            firstPass ? hdrColorBuf : pingpongColorbuffers[horizontal ? 0 : 1]);

        glBindVertexArray(screenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        horizontal = !horizontal;
        firstPass = false;
    }

    GLuint bloomTex = pingpongColorbuffers[horizontal ? 0 : 1];

    // ── Final HDR tone-map composite ──────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    hdrProg.use();
    hdrProg.setUniform("sceneTex", 0);
    hdrProg.setUniform("bloomBlurTex", 1);
    hdrProg.setUniform("hdrEnabled",      hdrEnabled      ? 1 : 0);
    hdrProg.setUniform("bloomEnabled",    bloomEnabled    ? 1 : 0);
    hdrProg.setUniform("filmMode",        filmMode        ? 1 : 0);
    hdrProg.setUniform("uNightVision",    nightVisionMode ? 1 : 0);
    hdrProg.setUniform("uTime", t);

    // Danger adds only subtle exposure boost — was 0.70 causing washout
    float finalExposure = exposure + dangerLevel * 0.25f;
    if (gameLost && fadeState == 7) finalExposure = 0.40f;
    if (gameWon && fadeState == 5) finalExposure = 1.80f;
    // Hyperspace streak phase: ramp to white (0-2.5s), calm after
    if (fadeState == 4 && endStateTimer < 2.5f) {
        float t4 = glm::clamp(endStateTimer / 2.5f, 0.0f, 1.0f);
        finalExposure = glm::mix(finalExposure, 14.0f, t4 * t4);
    }
    if (fadeState == 4 && endStateTimer >= 5.2f)
        finalExposure = 1.05f;
    // BH fall: ramp to extreme red exposure during approach (0-6s)
    if (fadeState == 6 && endStateTimer < 6.5f) {
        float ft = glm::clamp(endStateTimer / 6.5f, 0.0f, 1.0f);
        finalExposure = glm::mix(finalExposure, 3.8f, ft * ft);
    }
    hdrProg.setUniform("exposure", finalExposure);

    // BH fall distortion factor: ramps 0→1 over the first 4 s of the fall
    float bhFallDistort = (fadeState == 6)
        ? glm::clamp(endStateTimer / 4.0f, 0.0f, 1.0f) : 0.0f;
    hdrProg.setUniform("uFadeAlpha",     fadeAlpha);
    hdrProg.setUniform("uBHFallDistort", bhFallDistort);
    hdrProg.setUniform("uDustNearby",    dustNearbyDensity);

    // BH proximity distortion: ramps with danger level but stops once any
    // end-state animation begins (fadeState 5 = win screen, 6 = BH fall cinematic,
    // 7 = lose screen). uBHFallDistort takes over during the fall itself.
    float bhProximity = (fadeState < 5 && bhFallDistort == 0.0f)
        ? glm::clamp(dangerLevel * dangerLevel * 1.4f, 0.0f, 1.0f)
        : 0.0f;
    hdrProg.setUniform("uBHProximity", bhProximity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuf);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTex);

    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    drawOverlayUI();
}

// ─── resize ──────────────────────────────────────────────────────────────────
void SceneBasic_Uniform::resize(int w, int h)
{
    width = w;
    height = h;

    glViewport(0, 0, width, height);

    if (hdrColorBuf) glDeleteTextures(1, &hdrColorBuf);
    if (rboDepth) glDeleteRenderbuffers(1, &rboDepth);
    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);

    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongColorbuffers);

    hdrFBO = 0;
    hdrColorBuf = 0;
    rboDepth = 0;
    pingpongFBO[0] = pingpongFBO[1] = 0;
    pingpongColorbuffers[0] = pingpongColorbuffers[1] = 0;

    setupHDRFramebuffer();
}