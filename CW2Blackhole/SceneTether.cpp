#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include <iostream>

// ─── initRopeNodes ───────────────────────────────────────────────────────────
void SceneBasic_Uniform::initRopeNodes()
{
    for (int i = 0; i <= ROPE_SEGMENTS; ++i)
    {
        ropeNodes[i] = shipPosition;
        ropeVelocities[i] = glm::vec3(0.0f);
    }
}

// ─── updateRope ──────────────────────────────────────────────────────────────
// Simple Verlet rope: node 0 is pinned to ship, node N is pinned to player
void SceneBasic_Uniform::updateRope(float dt)
{
    if (inCockpit) return;

    // gravity = -1.2 m/s² per simulation unit: intentionally weaker than Earth
    // (~9.81) so the rope sags visibly but does not whip chaotically at the
    // EVA speed scale.  Chosen empirically for comfortable play feel.
    const float gravity = -1.2f;
    const float segLen  = tetherMaxDistance / (float)ROPE_SEGMENTS;
    // damping = 0.92 per frame: equivalent to ~65% energy retention per second
    // at 60 fps (0.92^60 ≈ 0.007) — heavy enough to kill oscillations quickly
    // without the rope looking stiff.  Standard Verlet damping range is 0.85–0.98;
    // Reference: Jakobsen, T. (2001). "Advanced Character Physics." GDC 2001.
    const float damping = 0.92f;

    // Pin ends — rope attaches to ship at node 0, and to the player's
    // shoulder/chest (slightly below and in front of the eye) at node N
    // so the connection is visible rather than clipping into the view frustum.
    glm::vec3 playerAnchor = camera.Position
                           - camera.Up    * 1.1f
                           + camera.Front * 0.4f;
    ropeNodes[0]            = shipPosition;
    ropeNodes[ROPE_SEGMENTS] = playerAnchor;

    // Apply velocity + gravity
    for (int i = 1; i < ROPE_SEGMENTS; ++i)
    {
        glm::vec3 vel = ropeVelocities[i];
        vel.y += gravity * dt;
        vel *= damping;
        ropeNodes[i] += vel * dt;
        ropeVelocities[i] = vel;
    }

    // Satisfy distance constraints (a few iterations)
    for (int iter = 0; iter < 6; ++iter)
    {
        for (int i = 0; i < ROPE_SEGMENTS; ++i)
        {
            glm::vec3 delta = ropeNodes[i + 1] - ropeNodes[i];
            float len = glm::length(delta);
            if (len < 0.0001f) continue;
            float correction = (len - segLen) * 0.5f;
            glm::vec3 dir = delta / len;
            if (i > 0)
                ropeNodes[i] += dir * correction;
            if (i + 1 < ROPE_SEGMENTS)
                ropeNodes[i + 1] -= dir * correction;
        }
        // re-pin ends after each solve
        ropeNodes[0]             = shipPosition;
        ropeNodes[ROPE_SEGMENTS] = playerAnchor;
    }
}

// ─── renderTetherRope ────────────────────────────────────────────────────────
// Uses tetherProg (tether.vert + tether.geom + tether.frag).
// The geometry shader extrudes each line segment into a camera-facing ribbon
// quad, producing a thick glowing rope regardless of driver glLineWidth caps.
void SceneBasic_Uniform::renderTetherRope(const glm::mat4& view, const glm::mat4& proj)
{
    if (inCockpit) return;

    // Build flat float array from ropeNodes
    std::vector<float> pts;
    pts.reserve((ROPE_SEGMENTS + 1) * 3);
    for (int i = 0; i <= ROPE_SEGMENTS; ++i)
    {
        pts.push_back(ropeNodes[i].x);
        pts.push_back(ropeNodes[i].y);
        pts.push_back(ropeNodes[i].z);
    }

    glBindVertexArray(tetherVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tetherVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pts.size() * sizeof(float), pts.data());

    // Colour: cyan when slack → red when at limit
    glm::vec3 ropeGlow = glm::mix(
        glm::vec3(0.10f, 0.65f, 1.00f),   // slack: NMS cyan
        glm::vec3(1.00f, 0.18f, 0.10f),   // taut: warning red
        tetherWarning
    );

    // Ribbon widens slightly when taut so the warning reads clearly
    float halfWidth    = glm::mix(0.40f, 0.65f, tetherWarning);
    float glowStrength = 2.8f + tetherWarning * 4.0f;

    // Stability-window pulse: 1.0 when the marker is inside the hit zone
    float minigamePulse = 0.0f;
    if (minigameState == TetherMinigameState::Active)
    {
        float dist = std::abs(tether.markerPos - tether.zoneCenter);
        if (dist < tether.zoneHalfWidth)
            minigamePulse = 1.0f - dist / tether.zoneHalfWidth;  // smooth 0→1 at zone edge
    }
    // Keep a flash on successful press
    minigamePulse = glm::max(minigamePulse, tether.successPulse);

    tetherProg.use();
    tetherProg.setUniform("uViewProj",      proj * view);
    tetherProg.setUniform("uCamPos",        camera.Position);
    tetherProg.setUniform("uHalfWidth",     halfWidth);
    tetherProg.setUniform("uGlowColor",     ropeGlow);
    tetherProg.setUniform("uGlowStrength",  glowStrength);
    tetherProg.setUniform("uTime",          (float)glfwGetTime());
    tetherProg.setUniform("uMinigamePulse", minigamePulse);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive glow
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glDrawArrays(GL_LINE_STRIP, 0, ROPE_SEGMENTS + 1);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    glBindVertexArray(0);
}

// ─── startTetherMinigame ─────────────────────────────────────────────────────
void SceneBasic_Uniform::startTetherMinigame()
{
    if (minigameState != TetherMinigameState::Inactive) return;

    minigameState = TetherMinigameState::Active;
    tether.markerPos = 0.08f;
    tether.markerSpeed = 0.42f;   // slow start — ramps non-linearly with locks
    tether.markerDir = 1.0f;
    tether.zoneCenter = 0.50f;
    tether.zoneHalfWidth = 0.20f;   // wider start, narrows per lock
    tether.pressCount = 0;
    tether.expectingA = true;
    tether.successPulse = 0.0f;
    tether.failFlash = 0.0f;
    tether.launchProgress = 0.0f;
    tether.wavePhase = 0.0f;
    tether.tensionPct = glm::clamp(
        glm::length(camera.Position - shipPosition)
        / glm::max(0.001f, tetherMaxDistance),
        0.0f, 1.0f);
    tether.reelRemaining = glm::length(camera.Position - shipPosition);

    std::cout << "[EVA] Tether Stabilisation! Match the marker to the window (x5).\n";
}

// ─── updateTetherMinigame ─────────────────────────────────────────────────────
// Upgraded oscilloscope minigame (CW2):
//   • 5 locks needed (was 3) — more sustained skill required
//   • Marker speed ramps non-linearly per lock (0.75 → ~2.4 by lock 5)
//   • Stability window DRIFTS to a new position after each lock
//   • Wrong-key press = harsh penalty (marker jumps to opposite end)
//   • Missed-window = softer penalty (lose a lock, window slightly widens)
void SceneBasic_Uniform::updateTetherMinigame(float dt)
{
    if (minigameState != TetherMinigameState::Active) return;

    GLFWwindow* w = glfwGetCurrentContext();

    const int LOCKS_NEEDED = 5;

    // Waveform phase — faster with tension and lock count
    float waveSpeed = 2.0f + tether.pressCount * 0.55f + tether.tensionPct * 1.2f;
    tether.wavePhase += dt * waveSpeed;

    // Marker speed: non-linear ramp — slow start, builds to challenging by lock 5
    tether.markerSpeed = 0.42f + tether.pressCount * tether.pressCount * 0.09f;
    tether.markerPos += tether.markerDir * tether.markerSpeed * dt;

    if (tether.markerPos >= 1.0f) { tether.markerPos = 1.0f; tether.markerDir = -1.0f; }
    if (tether.markerPos <= 0.0f) { tether.markerPos = 0.0f; tether.markerDir = 1.0f; }

    // Tension bar
    tether.tensionPct = glm::clamp(
        glm::length(camera.Position - shipPosition)
        / glm::max(0.001f, tetherMaxDistance),
        0.0f, 1.0f);

    // Decay feedback flashes
    tether.successPulse = glm::max(0.0f, tether.successPulse - dt * 3.2f);
    tether.failFlash = glm::max(0.0f, tether.failFlash - dt * 3.2f);

    // Edge-detect A and D.  prevTetherA/D are member variables so they reset
    // correctly when the game restarts (function-local statics would not).
    bool aNow = glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
    bool dNow = glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;

    bool pressedCorrectKey = false;
    bool pressedWrongKey = false;
    bool anyPress = false;

    if (aNow && !prevTetherA) {
        anyPress = true;
        pressedCorrectKey = tether.expectingA;
        pressedWrongKey = !tether.expectingA;
    }
    if (dNow && !prevTetherD) {
        anyPress = true;
        pressedCorrectKey = !tether.expectingA;
        pressedWrongKey = tether.expectingA;
    }
    prevTetherA = aNow;
    prevTetherD = dNow;

    if (anyPress)
    {
        if (pressedWrongKey)
        {
            // Harsh: flash + jump marker to opposite end
            tether.failFlash = 1.0f;
            tether.markerPos = (tether.markerDir > 0.0f) ? 0.92f : 0.08f;
            tether.markerDir *= -1.0f;
            tether.pressCount = glm::max(0, tether.pressCount - 1);
            tether.launchProgress = (float)tether.pressCount / LOCKS_NEEDED;
            std::cout << "[Tether] WRONG KEY — lock lost!\n";
        }
        else if (pressedCorrectKey)
        {
            bool inZone = fabsf(tether.markerPos - tether.zoneCenter) < tether.zoneHalfWidth;

            if (inZone)
            {
                tether.pressCount++;
                tether.launchProgress = (float)tether.pressCount / LOCKS_NEEDED;
                tether.successPulse = 1.0f;
                tether.expectingA = !tether.expectingA;

                // Zone narrows with each lock
                tether.zoneHalfWidth = glm::max(0.07f, tether.zoneHalfWidth - 0.025f);

                // Zone drifts to a new random-ish centre after each lock
                float newCenter = 0.25f + 0.50f *
                    fmodf(sinf((float)tether.pressCount * 127.1f +
                        tether.wavePhase * 0.1f) * 0.5f + 0.5f, 1.0f);
                tether.zoneCenter = glm::clamp(newCenter,
                    tether.zoneHalfWidth + 0.05f,
                    1.0f - tether.zoneHalfWidth - 0.05f);

                float fullDist = glm::length(camera.Position - shipPosition);
                tether.reelRemaining = glm::max(0.0f,
                    fullDist * (1.0f - tether.launchProgress));

                std::cout << "[Tether] Lock " << tether.pressCount
                    << "/" << LOCKS_NEEDED << " — "
                    << std::fixed << std::setprecision(1)
                    << tether.reelRemaining << " m remaining.\n";

                // Pull player toward ship by one segment on each successful lock
                {
                    float dist = glm::length(camera.Position - shipPosition);
                    float reelStep = dist / (float)(LOCKS_NEEDED - tether.pressCount + 1);
                    if (dist > 0.5f) {
                        glm::vec3 toShip = glm::normalize(shipPosition - camera.Position);
                        camera.Position += toShip * reelStep;
                    }
                    // Lock the new closer distance — gravity cannot drag past this
                    tetherLockedDist = glm::length(camera.Position - shipPosition);
                }

                if (tether.pressCount >= LOCKS_NEEDED)
                {
                    minigameState = TetherMinigameState::Success;
                    std::cout << "[Tether] Fully stabilised — reeling in!\n";
                }
            }
            else
            {
                // Wrong press: push player back and update the lock ceiling
                tether.failFlash = 0.75f;
                tether.pressCount = glm::max(0, tether.pressCount - 1);
                tether.launchProgress = (float)tether.pressCount / LOCKS_NEEDED;
                tether.zoneHalfWidth = glm::min(0.20f, tether.zoneHalfWidth + 0.012f);
                float pushDist = glm::length(camera.Position - shipPosition);
                float maxPush = tetherMaxDistance - pushDist;
                if (maxPush > 0.5f) {
                    glm::vec3 fromShip = glm::normalize(camera.Position - shipPosition);
                    float pushAmt = glm::min(maxPush * 0.5f, 6.0f);
                    camera.Position += fromShip * pushAmt;
                }
                // Raise the locked ceiling to the new (further) position
                tetherLockedDist = glm::length(camera.Position - shipPosition);
                std::cout << "[Tether] Misaligned — tether recoils!\n";
            }
        }
    }

    // Auto-complete if player swims back to ship
    if (glm::length(camera.Position - shipPosition) < shipReentryDistance)
        minigameState = TetherMinigameState::Success;
}

void SceneBasic_Uniform::drawTetherMinigameUI()
{
    if (minigameState != TetherMinigameState::Active) return;

    float nowT = (float)glfwGetTime();

    // ── scissor-rect helper (same as drawOverlayUI) ───────────────────────────
    auto drawRect = [&](int x, int y, int w, int h, float r, float g, float b, float a = 1.0f)
        {
            if (w <= 0 || h <= 0) return;
            glScissor(x, y, w, h);
            glClearColor(r * a, g * a, b * a, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        };

    auto drawFrame = [&](int x, int y, int w, int h, float r, float g, float b)
        {
            drawRect(x, y, w, 2, r, g, b);
            drawRect(x, y + h - 2, w, 2, r, g, b);
            drawRect(x, y, 2, h, r, g, b);
            drawRect(x + w - 2, y, 2, h, r, g, b);
        };

    // Bitmap text (reuse glyph system from drawOverlayUI)
    auto drawBitmapChar = [&](int x, int y, char c, int s, float r, float g, float b)
        {
            if (c == ' ') return;
            const char* glyph = getBitmapGlyph((char)toupper((unsigned char)c));
            for (int row = 0; row < 5; ++row)
                for (int col = 0; col < 3; ++col)
                    if (bitmapGlyphPixel(glyph, row, col))
                        drawRect(x + col * s, y + (4 - row) * s, s, s, r, g, b);
        };
    auto drawText = [&](int x, int y, const std::string& txt, int s, float r, float g, float b)
        {
            int pen = x;
            for (char c : txt) {
                drawBitmapChar(pen, y, c, s, r, g, b);
                pen += (c == ' ') ? (2 * s) : (4 * s);
            }
        };

    // ── Panel geometry ────────────────────────────────────────────────────────
    const int PW = 560;
    const int PH = 220;
    const int PX = width / 2 - PW / 2;
    const int PY = height / 2 - PH / 2 - 80;  // slightly above centre

    // Dark background
    drawRect(PX, PY, PW, PH, 0.02f, 0.04f, 0.10f, 0.92f);

    // Cyan outer border
    drawFrame(PX, PY, PW, PH, 0.18f, 0.82f, 1.00f);

    // Title bar gradient strip
    drawRect(PX + 2, PY + PH - 28, PW - 4, 26, 0.06f, 0.24f, 0.52f);
    drawText(PX + 10, PY + PH - 22, "TETHER STABILISATION", 3, 0.80f, 0.96f, 1.00f);

    // ── TENSION bar ───────────────────────────────────────────────────────────
    const int TBAR_X = PX + 16;
    const int TBAR_W = PW - 32;
    const int TBAR_H = 12;
    const int TBAR_Y = PY + PH - 54;

    drawText(TBAR_X, TBAR_Y + TBAR_H + 4, "TENSION", 3, 0.55f, 0.90f, 1.00f);

    // Bar background
    drawRect(TBAR_X, TBAR_Y, TBAR_W, TBAR_H, 0.04f, 0.06f, 0.12f);
    drawFrame(TBAR_X, TBAR_Y, TBAR_W, TBAR_H, 0.14f, 0.78f, 1.00f);

    // Tension fill (cyan → red with tension)
    float tPct = tether.tensionPct;
    int tensionFill = (int)((TBAR_W - 4) * glm::clamp(tPct, 0.0f, 1.0f));
    if (tensionFill > 0)
        drawRect(TBAR_X + 2, TBAR_Y + 2, tensionFill, TBAR_H - 4,
            glm::mix(0.18f, 1.00f, tPct),
            glm::mix(0.82f, 0.22f, tPct),
            glm::mix(1.00f, 0.10f, tPct));

    // Tension % label
    std::ostringstream tensStr;
    tensStr << (int)(tPct * 100.0f) << "%";
    drawText(TBAR_X + TBAR_W + 6, TBAR_Y, tensStr.str(), 3,
        glm::mix(0.30f, 1.00f, tPct),
        glm::mix(0.90f, 0.22f, tPct),
        0.30f);

    // ── Oscilloscope waveform track ───────────────────────────────────────────
    const int OSC_X = PX + 56;          // leave room for A key label
    const int OSC_W = PW - 112;         // leave room for D key label
    const int OSC_H = 52;
    const int OSC_Y = PY + 56;

    // Track background
    drawRect(OSC_X, OSC_Y, OSC_W, OSC_H, 0.03f, 0.05f, 0.10f);
    drawFrame(OSC_X, OSC_Y, OSC_W, OSC_H, 0.14f, 0.55f, 0.82f);

    // Stability window (the green zone)
    int zoneX = OSC_X + (int)((tether.zoneCenter - tether.zoneHalfWidth) * OSC_W);
    int zoneW2 = (int)(tether.zoneHalfWidth * 2.0f * OSC_W);
    float zA = 0.60f + 0.25f * sinf(nowT * 5.0f);   // pulse
    drawRect(zoneX, OSC_Y + 2, zoneW2, OSC_H - 4, 0.04f, 0.30f * zA, 0.12f * zA);
    // Green zone borders
    drawRect(zoneX, OSC_Y, 2, OSC_H, 0.12f, 1.00f, 0.42f);
    drawRect(zoneX + zoneW2 - 2, OSC_Y, 2, OSC_H, 0.12f, 1.00f, 0.42f);

    // Animated sine waveform drawn as thin horizontal slices
    // We paint one-pixel-tall rows of the wave using narrow drawRects
    {
        const int WAVE_STEPS = OSC_W;
        int midY = OSC_Y + OSC_H / 2;
        float amp = (OSC_H / 2) - 4.0f;
        // Waveform: combination of two sines to look like the reference image
        for (int px = 0; px < WAVE_STEPS; px += 2)
        {
            float fx = (float)px / WAVE_STEPS;
            float wave = sinf(fx * 20.0f + tether.wavePhase) * 0.55f
                + sinf(fx * 8.5f + tether.wavePhase * 0.7f) * 0.35f
                + sinf(fx * 3.2f + tether.wavePhase * 0.3f) * 0.10f;
            int wy = midY + (int)(wave * amp);
            wy = glm::clamp(wy, OSC_Y + 2, OSC_Y + OSC_H - 3);

            // In-zone: brighter cyan; out-of-zone: dim blue
            bool inZ = (fx > (tether.zoneCenter - tether.zoneHalfWidth)) &&
                (fx < (tether.zoneCenter + tether.zoneHalfWidth));
            drawRect(OSC_X + px, wy, 2, 2,
                inZ ? 0.25f : 0.08f,
                inZ ? 0.90f : 0.35f,
                inZ ? 1.40f : 0.80f);
        }
    }

    // Sweep marker (vertical line)
    {
        int mxPx = OSC_X + (int)(tether.markerPos * OSC_W);
        bool inZone = fabsf(tether.markerPos - tether.zoneCenter) < tether.zoneHalfWidth;

        // Vertical line
        drawRect(mxPx - 1, OSC_Y, 3, OSC_H,
            inZone ? 0.20f : 1.00f,
            inZone ? 1.00f : 0.42f,
            inZone ? 0.50f : 0.08f);

        // Marker diamond cap
        drawRect(mxPx - 3, OSC_Y + OSC_H - 6, 7, 6,
            inZone ? 0.20f : 1.00f,
            inZone ? 1.00f : 0.42f,
            inZone ? 0.50f : 0.08f);
    }

    // ── A key label (left) / D key label (right) ──────────────────────────────
    bool showA = tether.expectingA;

    // A key bracket
    int keyY = OSC_Y + (OSC_H / 2) - 12;
    int keyH2 = 24;
    drawRect(PX + 12, keyY, 40, keyH2,
        showA ? 0.08f : 0.03f,
        showA ? 0.60f : 0.14f,
        showA ? 1.20f : 0.20f);
    drawText(PX + 20, keyY + 6, "A", 4,
        showA ? 0.80f : 0.35f,
        showA ? 1.00f : 0.50f,
        showA ? 1.50f : 0.60f);

    // D key bracket
    int dkX = PX + PW - 52;
    drawRect(dkX, keyY, 40, keyH2,
        !showA ? 0.08f : 0.03f,
        !showA ? 0.60f : 0.14f,
        !showA ? 1.20f : 0.20f);
    drawText(dkX + 8, keyY + 6, "D", 4,
        !showA ? 0.80f : 0.35f,
        !showA ? 1.00f : 0.50f,
        !showA ? 1.50f : 0.60f);

    // ── "ALIGN WITH THE STABILITY WINDOW" label ───────────────────────────────
    drawText(PX + 16, OSC_Y - 18, "ALIGN WITH THE STABILITY WINDOW", 3,
        0.25f, 0.88f, 0.55f);

    // ── Lock pips (filled circles drawn as small squares) ────────────────────
    const int MAX_PIPS = 5;    // 5 locks needed — matches updateTetherMinigame
    int pipRowX = PX + 16;
    int pipRowY = PY + 30;
    int pipSize = 12;
    for (int i = 0; i < MAX_PIPS; ++i)
    {
        bool locked = i < tether.pressCount;
        bool nextUp = i == tether.pressCount;
        drawRect(pipRowX + i * (pipSize + 4), pipRowY, pipSize, pipSize,
            locked ? 0.15f : (nextUp ? 0.30f : 0.04f),
            locked ? 0.88f : (nextUp ? 0.60f : 0.14f),
            locked ? 0.40f : (nextUp ? 0.90f : 0.20f));
    }

    // ── Reel-in distance readout ──────────────────────────────────────────────
    {
        std::ostringstream oss;
        oss << "REELING IN...  "
            << std::fixed << std::setprecision(1)
            << tether.reelRemaining << " M REMAINING";
        drawText(PX + 16, PY + 12, oss.str(), 3, 0.60f, 0.92f, 1.00f);
    }

    // ── Feedback overlays ─────────────────────────────────────────────────────
    if (tether.successPulse > 0.01f)
        drawRect(PX, PY, PW, PH, 0.00f, 0.90f, 0.40f, tether.successPulse * 0.20f);
    if (tether.failFlash > 0.01f)
        drawRect(PX, PY, PW, PH, 0.90f, 0.10f, 0.05f, tether.failFlash * 0.25f);
}


