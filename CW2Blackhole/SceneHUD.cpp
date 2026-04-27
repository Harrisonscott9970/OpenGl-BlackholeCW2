#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <functional>



// Draws an Earth-like planet in 2D using the scissor rect system
// cx,cy = screen centre, ER = radius in pixels, alpha = opacity
static void drawEarthHelper(
    int cx, int cy, int ER, float alpha,
    std::function<void(int, int, int, int, float, float, float)> drawRect)
{
    if (ER < 4 || alpha < 0.01f) return;
    // Coarsen the step when the planet is small for performance
    int step = glm::max(3, 280 / glm::max(1, ER));

    // Sun direction (upper-left)
    const float LX = -0.36f, LY = -0.54f, LZ = 0.76f;

    // Loop every pixel inside the planet disc
    for (int row = -ER; row <= ER; row += step)
    {
        float rf = (float)row / ER;
        int   hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * ER);
        if (hw < 1) continue;

        for (int col = -hw; col <= hw; col += step)
        {
            float dx = (float)col / ER;   // -1..1 screen X on sphere
            float dy = rf;                 // -1..1 screen Y on sphere
            float dz = sqrtf(glm::max(0.0f, 1.0f - dx * dx - dy * dy));

            // Spherical UV coordinates
            float lon = atan2f(dx, dz);
            float lat = asinf(glm::clamp(dy, -1.0f, 1.0f));
            float u = (lon + 3.14159f) / 6.28318f;
            float v = (lat + 1.5708f) / 3.14159f;

            // Lighting with a soft day/night terminator and limb darkening
            float ndotl = glm::clamp(LX * dx + LY * dy + LZ * dz, 0.0f, 1.0f);
            float light = glm::smoothstep(0.02f, 0.35f, ndotl);
            float limbD = 0.50f + 0.50f * dz;
            float shade = light * limbD;

            // Continent mask — overlapping sine waves give a realistic coastline shape
            float a1 = u * 6.28318f, a2 = v * 3.14159f;
            float land =
                sinf(a1 * 1.70f + 0.9f) * cosf(a2 * 2.30f + 0.4f) * 1.00f +
                cosf(a1 * 2.90f - 1.3f) * sinf(a2 * 1.60f + 1.8f) * 0.70f +
                sinf(a1 * 4.10f + 2.1f) * cosf(a2 * 0.80f - 0.6f) * 0.50f +
                cosf(a1 * 0.95f + 0.5f) * sinf(a2 * 3.50f + 0.2f) * 0.45f +
                sinf(a1 * 6.30f - 0.7f) * cosf(a2 * 1.20f + 2.5f) * 0.30f +
                cosf(a1 * 3.70f + 1.6f) * cosf(a2 * 4.10f - 1.1f) * 0.25f;
            bool isLand = (land > 0.55f);

            // Polar ice caps at the top and bottom
            bool polarIce = (v > 0.88f || v < 0.12f);
            if (polarIce) isLand = true;

            // Split land into biomes by latitude
            float vMid = fabsf(v - 0.50f) * 2.0f;  // 0 = equator, 1 = pole
            bool isDesert = isLand && !polarIce && (land > 0.90f) && (vMid < 0.55f);
            bool isMountain = isLand && !polarIce && (land > 1.30f);
            bool isTundra = isLand && !polarIce && (vMid > 0.65f);

            // Cloud mask using different wave frequencies so it doesn't match land
            float c1 = u * 6.28318f, c2 = v * 3.14159f;
            float cloudVal =
                cosf(c1 * 3.15f + c2 * 1.80f + 0.7f) * 0.55f +
                sinf(c1 * 1.40f - c2 * 3.20f + 2.1f) * 0.40f +
                cosf(c1 * 5.20f + c2 * 0.60f - 1.2f) * 0.25f +
                sinf(c1 * 2.05f + c2 * 2.40f + 1.5f) * 0.20f;
            // Fewer clouds near the poles
            float cloudBias = 1.0f - 0.5f * vMid * vMid;
            bool isCloudy = (cloudVal * cloudBias > 0.30f);

            // Pick the pixel colour based on biome
            float r2, g2, b2;

            if (polarIce)
            {
                // Ice cap — bright blue-white
                float iceBright = 0.82f + 0.16f * (land - 0.55f) * 0.5f;
                r2 = glm::mix(0.30f, iceBright, shade);
                g2 = glm::mix(0.34f, iceBright * 0.96f, shade);
                b2 = glm::mix(0.42f, iceBright * 1.00f, shade);
            }
            else if (isMountain)
            {
                // Rocky mountains with snow caps
                r2 = glm::mix(0.06f, 0.62f, shade);
                g2 = glm::mix(0.06f, 0.60f, shade);
                b2 = glm::mix(0.08f, 0.58f, shade);
            }
            else if (isDesert)
            {
                // Desert — sandy orange
                r2 = glm::mix(0.08f, 0.76f, shade);
                g2 = glm::mix(0.05f, 0.55f, shade);
                b2 = glm::mix(0.02f, 0.20f, shade);
            }
            else if (isTundra)
            {
                // Cold tundra — dark olive green
                r2 = glm::mix(0.03f, 0.18f, shade);
                g2 = glm::mix(0.05f, 0.30f, shade);
                b2 = glm::mix(0.02f, 0.10f, shade);
            }
            else if (isLand)
            {
                // Tropical/temperate land — green
                float greenness = glm::clamp(1.0f - vMid * 1.4f, 0.0f, 1.0f);
                r2 = glm::mix(0.03f, glm::mix(0.28f, 0.18f, greenness), shade);
                g2 = glm::mix(0.06f, glm::mix(0.50f, 0.38f, greenness), shade);
                b2 = glm::mix(0.01f, glm::mix(0.12f, 0.08f, greenness), shade);
            }
            else
            {
                // Ocean — deep blue with a sun glint
                float coastProx = glm::clamp(1.0f - (land - 0.55f) / 0.6f, 0.0f, 1.0f);
                float spec = powf(ndotl, 22.0f) * light;
                float deepR = 0.04f + spec * 0.50f;
                float deepG = 0.14f + coastProx * 0.06f + spec * 0.38f;
                float deepB = 0.42f + coastProx * 0.10f + spec * 0.25f;
                r2 = glm::mix(0.01f, deepR, shade);
                g2 = glm::mix(0.04f, deepG, shade);
                b2 = glm::mix(0.10f, deepB, shade);
            }

            // Blend in clouds on the lit side
            if (isCloudy)
            {
                float cf = glm::clamp((cloudVal * cloudBias - 0.30f) / 0.45f, 0.0f, 1.0f)
                    * light * 0.90f;
                r2 = glm::mix(r2, 0.92f, cf);
                g2 = glm::mix(g2, 0.94f, cf);
                b2 = glm::mix(b2, 0.97f, cf);
            }

            drawRect(cx + col, cy + row, step, step, r2 * alpha, g2 * alpha, b2 * alpha);
        }
    }

    // Atmosphere glow ring around the planet edge
    int atmosMax = (int)(ER * 1.14f);
    for (int row = -atmosMax; row <= atmosMax; row += step)
    {
        float rf = (float)row / atmosMax;
        int   hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * atmosMax);
        int   hw2 = (int)(sqrtf(glm::max(0.0f, 1.0f - (float)(row * row) / (float)(ER * ER))) * ER);
        int   ow = glm::max(0, hw - glm::max(0, hw2));
        if (ow < 1) continue;
        // Fade at edges; lit side is slightly brighter
        float edgeFrac = (float)(hw2) / glm::max(1, hw);
        float ha = (edgeFrac) * (1.0f - fabsf(rf) * 0.55f) * 0.70f * alpha;
        float aLight = 0.55f + 0.45f * glm::clamp(-LY * rf + LZ * 0.8f, 0.0f, 1.0f);
        float aR = 0.22f * ha * aLight;
        float aG = 0.52f * ha * aLight;
        float aB = ha * aLight;
        drawRect(cx - hw, cy + row, ow, step, aR, aG, aB);
        drawRect(cx + hw2, cy + row, ow, step, aR, aG, aB);
    }
}


// ─── drawOverlayUI ───────────────────────────────────────────────────────────
void SceneBasic_Uniform::drawOverlayUI()
{
    auto drawRect = [&](int x, int y, int w, int h, float r, float g, float b)
        {
            if (w <= 0 || h <= 0) return;
            glScissor(x, y, w, h);
            glClearColor(r, g, b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        };

    auto drawFrame = [&](int x, int y, int w, int h, float r, float g, float b)
        {
            drawRect(x, y, w, 2, r, g, b);
            drawRect(x, y + h - 2, w, 2, r, g, b);
            drawRect(x, y, 2, h, r, g, b);
            drawRect(x + w - 2, y, 2, h, r, g, b);
        };

    auto drawBitmapChar = [&](int x, int y, char c, int s, float r, float g, float b)
        {
            if (c == ' ') return;
            const char* glyph = getBitmapGlyph((char)toupper((unsigned char)c));
            for (int row = 0; row < 5; ++row)
                for (int col = 0; col < 3; ++col)
                    if (bitmapGlyphPixel(glyph, row, col))
                        drawRect(x + col * s, y + (4 - row) * s, s, s, r, g, b);
        };

    auto drawBitmapText = [&](int x, int y, const std::string& txt, int s, float r, float g, float b)
        {
            int pen = x;
            for (char c : txt)
            {
                drawBitmapChar(pen, y, c, s, r, g, b);
                pen += (c == ' ') ? (2 * s) : (4 * s);
            }
        };

    auto drawBar = [&](int x, int y, int w, int h, float value, glm::vec3 fill, const std::string& label)
        {
            drawRect(x, y, w, h, 0.02f, 0.04f, 0.08f);
            drawFrame(x, y, w, h, 0.14f, 0.78f, 1.00f);
            int inner = std::max(0, w - 4);
            int fillW = (int)(inner * glm::clamp(value, 0.0f, 1.0f));
            drawRect(x + 2, y + 2, fillW, h - 4, fill.r, fill.g, fill.b);
            drawBitmapText(x, y + h + 5, label, 3, 0.55f, 0.90f, 1.00f);
        };

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    int total     = static_cast<int>(energyCells.size());
    int collected = collectedCells;   // maintained by updatePickupAnimations()

    float nowT = (float)glfwGetTime();
    float missionElapsed2 = gameWon
        ? missionFinalTime
        : glm::max(0.0f, nowT - missionStartTime);
    int displayElapsed = (int)glm::round(missionElapsed2 * timeDilationFactor);
    int mins = displayElapsed / 60;
    int secs = displayElapsed % 60;

    std::ostringstream timer;
    timer << std::setw(2) << std::setfill('0') << mins << ":" << std::setw(2) << std::setfill('0') << secs;

    // Hide regular HUD during BH fall cinematic (fadeState 6 = cutscene, 7 = lost screen)
    bool showHUD = (fadeState != 6 && fadeState != 7 && fadeState != 4 && fadeState != 5);

    int margin = 18;
    int topY = height - 26;

    if (showHUD) {
    drawBitmapText(margin, topY, "CELLS " + std::to_string(collected) + "/" + std::to_string(total), 4, 0.68f, 0.95f, 1.00f);
    drawBitmapText(width / 2 - 42, topY, timer.str(), 4, 1.00f, 0.75f, 0.30f);
    drawBitmapText(width - 170, topY, inCockpit ? "BRIDGE" : "TETHER PILOT", 4, 0.72f, 1.00f, 0.78f);

    // Boost bar — red while hyperspace cooldown is active, normal cyan otherwise
    {
        bool onCooldown = (boostCooldownTimer > 0.0f);
        glm::vec3 boostFill = onCooldown
            ? glm::vec3(1.00f, 0.12f, 0.06f)
            : glm::vec3(0.16f, 0.86f, 1.00f);
        float boostDisplay = onCooldown
            ? (boostCooldownTimer / BOOST_COOLDOWN_DURATION)  // drain toward zero as timer counts down
            : boostEnergy;
        drawBar(width - 220, height - 74, 190, 12, boostDisplay, boostFill,
                onCooldown ? "BOOST LOCK" : "BOOST");
        // Red lockout threshold tick at 25 %
        if (!onCooldown) {
            int threshX = (width - 220) + 2 + (int)((190 - 4) * 0.25f);
            drawRect(threshX, height - 74, 2, 12, 1.00f, 0.10f, 0.10f);
        }
        // Cooldown seconds remaining
        if (onCooldown) {
            int secs = (int)glm::ceil(boostCooldownTimer);
            drawBitmapText(width - 220, height - 74 - 18,
                std::to_string(secs) + "S", 3, 1.00f, 0.18f, 0.06f);
        }
    }
    drawBar(width - 220, height - 116, 190, 12, dangerLevel, glm::vec3(1.00f, 0.35f, 0.12f), "DANGER");
    }

    if (showHUD && !inCockpit)
    {
        float tetherDist = glm::length(camera.Position - shipPosition);
        float tetherPct = glm::clamp(tetherDist / glm::max(0.001f, tetherMaxDistance), 0.0f, 1.0f);
        drawBar(width - 220, height - 158, 190, 12, 1.0f - tetherPct, glm::vec3(0.25f, 1.00f, 0.52f), "TETHER");
    }

    // Top-left pickup pips.
    if (showHUD)
    for (int i = 0; i < total; ++i)
    {
        bool on = i < collected;
        float glow = on ? (0.82f + 0.18f * pickupPulse) : 1.0f;
        drawRect(14 + i * 24, height - 40, 16, 16, on ? 0.20f : 0.03f, on ? 0.88f * glow : 0.10f, on ? 1.00f * glow : 0.16f);
    }

    // Thin center reticle.
    if (showHUD) {
    int cx = width / 2;
    int cy = height / 2;
    drawRect(cx - 14, cy, 10, 2, 0.20f, 0.85f, 1.00f);
    drawRect(cx + 4, cy, 10, 2, 0.20f, 0.85f, 1.00f);
    drawRect(cx, cy - 14, 2, 10, 0.20f, 0.85f, 1.00f);
    drawRect(cx, cy + 4, 2, 10, 0.20f, 0.85f, 1.00f);
    drawRect(cx - 1, cy - 1, 3, 3, 1.0f, 1.0f, 1.0f);
    }

    // Compact objective / score block.
    if (showHUD) {
    std::string objective = (collected < total) ? (inCockpit ? "FLY TO CELLS" : "COLLECT CELL") : "RETURN TO PLATFORM";
    drawBitmapText(margin, 18, objective, 4, 1.00f, 0.66f, 0.22f);
    drawBitmapText(margin, 38, "SCORE " + std::to_string(playerScore), 3, 0.60f, 0.92f, 1.00f);
    }

    if (showHUD && comboCount > 1 && comboTimer > 0.0f)
        drawBitmapText(margin, 56, "COMBO X" + std::to_string(comboCount), 3, 0.98f, 0.86f, 0.32f);

    if (showHUD && nearDiskSlipstream > 0.15f)
    {
        // Show slipstream intensity as a percentage
        int slipPct = (int)(nearDiskSlipstream * 100.0f);
        drawBitmapText(width / 2 - 100, 18,
            "SLIPSTREAM  " + std::to_string(slipPct) + "%",
            3, 0.30f, 1.00f, 0.72f);
        // Small bar below showing intensity
        int barW = (int)(nearDiskSlipstream * 120.0f);
        drawRect(width / 2 - 60, 14, barW, 3, 0.20f, 1.00f, 0.58f);
    }

    if (showHUD && timeDilationFactor < 0.95f)
    {
        // Show how much the timer is slowing
        int dilPct = (int)((1.0f - timeDilationFactor) * 100.0f);
        drawBitmapText(width / 2 - 96, 38,
            "TIME DILATION  -" + std::to_string(dilPct) + "%",
            3, 0.86f, 0.72f, 1.00f);
    }

    if (showHUD && !inCockpit)
        drawBitmapText(margin, 74, "X REENTER  LMB PICKUP", 3, 0.60f, 0.92f, 1.00f);
    else if (showHUD)
        drawBitmapText(margin, 74, "X EVA  SHIFT BOOST  SPACE JUMP", 3, 0.60f, 0.92f, 1.00f);

    // Hyperspace pip display - only shown in cockpit when warp isn't playing
    if (showHUD && inCockpit && hyperspaceWarpTimer <= 0.0f)
    {
        int cx = width / 2;

        // 3 circle pips in a triangle: left and right at base, middle higher
        const int PR = 8;   // pip radius in pixels
        int pipBaseY = (int)(height * 0.83f) + 5;  // circle centre Y (base row)
        int pipHighY = pipBaseY + 18;               // middle pip is higher on screen
        int leftX    = cx - 14;                     // circle centres (not rect corners)
        int rightX   = cx + 14;
        int midX     = cx;

        float chargePct = (float)hyperspaceSpacebar / (float)HYPERSPACE_PRESSES;
        bool  onCooldown = boostCooldownTimer > 0.0f;
        bool  charging   = hyperspaceSpacebar > 0;

        // Charging sequence: left lights first (0-33%), right next (33-66%), middle last (66-100%)
        float leftOn  = charging ? glm::clamp(chargePct / 0.33f,           0.0f, 1.0f) : 0.0f;
        float rightOn = charging ? glm::clamp((chargePct - 0.33f) / 0.33f, 0.0f, 1.0f) : 0.0f;
        float midOn   = charging ? glm::clamp((chargePct - 0.66f) / 0.34f, 0.0f, 1.0f) : 0.0f;

        // Draw one circular pip centred at (pcx, pcy)
        auto drawPip = [&](int pcx, int pcy, float chargeAmt, int chargeIndex)
        {
            bool hasCharge = (chargeIndex < hyperspaceCharges);
            float r, g, b;
            if (onCooldown && hasCharge) {
                float blink = 0.55f + 0.45f * sinf(nowT * 5.0f);
                r = blink; g = 0.08f * blink; b = 0.04f * blink;
            } else if (charging && chargeAmt > 0.01f) {
                // Lighting up during charge — cyan-white
                r = chargeAmt * 0.35f; g = chargeAmt * 0.90f; b = chargeAmt;
            } else if (hasCharge && !charging) {
                // Idle with charge available — dim blue
                r = 0.05f; g = 0.38f; b = 0.80f;
            } else {
                // No charge or unlit during sequence
                r = 0.04f; g = 0.04f; b = 0.06f;
            }
            // Filled disc
            for (int dy = -PR; dy <= PR; dy++)
            {
                int hw = (int)sqrtf(glm::max(0.0f, (float)(PR * PR - dy * dy)));
                if (hw > 0)
                    drawRect(pcx - hw, pcy + dy, hw * 2, 1, r, g, b);
            }
            // Bright rim one pixel wide
            float rr = glm::min(r * 2.2f, 1.0f);
            float rg = glm::min(g * 1.5f, 1.0f);
            float rb = glm::min(b * 1.3f, 1.0f);
            for (int dy = -PR; dy <= PR; dy++)
            {
                int outer = (int)sqrtf(glm::max(0.0f, (float)(PR * PR - dy * dy)));
                int inner = (int)sqrtf(glm::max(0.0f, (float)((PR - 1) * (PR - 1) - dy * dy)));
                if (outer > inner)
                {
                    drawRect(pcx - outer, pcy + dy, outer - inner, 1, rr, rg, rb);
                    drawRect(pcx + inner, pcy + dy, outer - inner, 1, rr, rg, rb);
                }
            }
        };

        drawPip(leftX,  pipBaseY, leftOn,  0);  // left  - lights first
        drawPip(rightX, pipBaseY, rightOn, 1);  // right - lights second
        drawPip(midX,   pipHighY, midOn,   2);  // middle (top of triangle) - lights last

        // Remaining jump count label
        if (!onCooldown)
            drawBitmapText(cx - 12, pipBaseY - 18,
                "x" + std::to_string(hyperspaceCharges), 2, 0.14f, 0.55f, 1.00f);
    }

    // ── Hyperspace warp animation overlay (full-screen, both modes) ──────────
    if (hyperspaceWarpTimer > 0.0f)
    {
        int cx  = width  / 2;
        int cy  = height / 2;

        // Animation progress: 0 = just started, 1 = just ended
        float total = HYPERSPACE_WARP_DURATION;
        float elapsed = total - hyperspaceWarpTimer;
        float prog  = elapsed / total;           // 0→1

        // ── PHASE 1 (prog 0 → 0.5): compression charge-up ────────────────────
        if (prog < 0.5f)
        {
            float p1 = prog / 0.5f;  // 0→1 over first half

            // Growing dark vignette from edges
            int vW = (int)(p1 * p1 * (float)(width  / 2) * 0.75f);
            int vH = (int)(p1 * p1 * (float)(height / 2) * 0.75f);
            float vA = p1 * 0.90f;
            drawRect(0,          0,           vW, height, 0.0f, 0.0f, vA * 0.02f);
            drawRect(width - vW, 0,           vW, height, 0.0f, 0.0f, vA * 0.02f);
            drawRect(0,          0,           width, vH,  0.0f, 0.0f, vA * 0.02f);
            drawRect(0,          height - vH, width, vH,  0.0f, 0.0f, vA * 0.02f);

            // Radial compression lines — short streaks converging on centre
            int numLines = 32;
            for (int li = 0; li < numLines; ++li)
            {
                float ang  = (float)li / (float)numLines * 6.28318f;
                float dist = (float)glm::min(width, height) * 0.5f * (1.0f - p1 * 0.55f);
                float len  = dist * 0.22f * p1;
                float brA  = p1 * p1 * (0.4f + 0.6f * fabsf(sinf(ang * 3.0f)));
                int   x0   = cx + (int)(cosf(ang) * dist);
                int   y0   = cy + (int)(sinf(ang) * dist);
                int   x1   = cx + (int)(cosf(ang) * (dist - len));
                int   y1   = cy + (int)(sinf(ang) * (dist - len));
                // Draw as small rects along the line
                int steps = glm::max(2, (int)(len / 5.0f));
                for (int si = 0; si < steps; ++si)
                {
                    float f = (float)si / steps;
                    int px = x0 + (int)((x1 - x0) * f);
                    int py = y0 + (int)((y1 - y0) * f);
                    drawRect(px - 1, py - 1, 3, 3,
                        brA * 0.40f, brA * 0.75f, brA * 1.00f);
                }
            }

            // Centre glow — builds up as the jump charges
            float cgR = 12.0f + p1 * p1 * 60.0f;
            for (int ri = (int)cgR; ri > 0; ri -= 3)
            {
                float rf = 1.0f - (float)ri / cgR;
                float cg = rf * rf * p1 * p1 * 0.70f;
                drawRect(cx - ri, cy - 1, ri * 2, 3,
                    cg * 0.30f, cg * 0.70f, cg * 1.00f);
                drawRect(cx - 1, cy - ri, 3, ri * 2,
                    cg * 0.30f, cg * 0.70f, cg * 1.00f);
            }

            // "JUMP" flash text centred
            if (p1 > 0.70f)
            {
                float ta = (p1 - 0.70f) / 0.30f;
                float flk = (sinf(nowT * 28.0f) > 0.0f) ? 1.0f : 0.35f;
                drawBitmapText(cx - 52, cy + 18, "JUMP ENGAGED",
                    4, ta * flk * 0.35f, ta * flk * 0.90f, ta * flk * 1.00f);
            }
        }

        // ── PHASE 2 (prog 0.5 → 1.0): hyperspace tunnel exit ─────────────────
        if (prog >= 0.5f)
        {
            float p2 = (prog - 0.5f) / 0.5f;  // 0→1 over second half

            // White flash at start of phase 2 (the actual jump moment)
            float flash = glm::max(0.0f, 1.0f - p2 * 3.0f);
            if (flash > 0.01f)
            {
                drawRect(0, 0, width, height, flash, flash, flash * 1.05f);
            }

            // Hyperspace streaks: long radial lines from centre fading out
            float streakLen = (float)glm::min(width, height) * 0.5f
                            * (0.40f + p2 * 0.80f);
            int numStreaks = 48;
            for (int li = 0; li < numStreaks; ++li)
            {
                float ang  = (float)li / (float)numStreaks * 6.28318f;
                float lenF = 0.55f + 0.45f * fabsf(sinf(ang * 5.0f + 0.8f));
                float sLen = streakLen * lenF;
                float brS  = (1.0f - p2) * (0.5f + 0.5f * lenF);

                // Draw streak from near-centre outward
                int steps = glm::max(3, (int)(sLen / 6.0f));
                for (int si = 0; si < steps; ++si)
                {
                    float f   = (float)si / steps;
                    float fBr = brS * (1.0f - f * 0.70f);
                    int   px  = cx + (int)(cosf(ang) * f * sLen);
                    int   py  = cy + (int)(sinf(ang) * f * sLen);
                    int   sz  = glm::max(1, (int)((1.0f - f) * 4.0f));
                    drawRect(px - sz/2, py - sz/2, sz, sz,
                        fBr * 0.55f, fBr * 0.85f, fBr * 1.00f);
                }
            }

            // Fade-out blue tint overlay
            float fadeOut = glm::clamp((p2 - 0.45f) / 0.55f, 0.0f, 1.0f);
            if (fadeOut > 0.01f)
            {
                drawRect(0, 0, width, height,
                    0.0f, fadeOut * 0.04f, fadeOut * 0.08f);
            }
        }
    }

    // Project world positions to screen pixels for cell/cloud markers
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.08f, 5000.0f);
    glm::mat4 vp   = proj * camera.getViewMatrix();

    // Returns the screen position of a world point; off-screen points are clamped to the edge
    auto projectWorld = [&](glm::vec3 wp, int& sx, int& sy, bool& onScreen) -> bool
    {
        glm::vec4 clip = vp * glm::vec4(wp, 1.0f);
        float nx, ny;
        if (clip.w <= 0.0f)
        {
            // Behind camera — flip so the arrow points the right way
            float iw = (fabsf(clip.w) > 0.0001f) ? (1.0f / -clip.w) : 0.0f;
            nx = -clip.x * iw * 0.5f + 0.5f;
            ny = -clip.y * iw * 0.5f + 0.5f;
            nx = (nx < 0.5f) ? 0.0f : 1.0f;
            ny = (ny < 0.5f) ? 0.0f : 1.0f;
            onScreen = false;
        }
        else
        {
            nx = (clip.x / clip.w) * 0.5f + 0.5f;
            ny = (clip.y / clip.w) * 0.5f + 0.5f;
            onScreen = (nx >= 0.02f && nx <= 0.98f && ny >= 0.02f && ny <= 0.98f);
        }
        int brd = 24;
        sx = glm::clamp((int)(nx * width),  brd, width  - brd);
        sy = glm::clamp((int)(ny * height), brd, height - brd);
        return true;
    };

    // Draw a small arrow pointing in (dx,dy) direction
    auto drawArrow = [&](int ax, int ay, int dx, int dy, float r, float g, float b)
    {
        int adx = (dx == 0) ? 0 : (dx > 0 ? 1 : -1);
        int ady = (dy == 0) ? 0 : (dy > 0 ? 1 : -1);
        // Shaft
        drawRect(ax - adx*6, ay - ady*6, 2 + abs(adx)*10, 2 + abs(ady)*10, r, g, b);
        // Head wings perpendicular to shaft
        if (adx != 0) {
            drawRect(ax + adx*5, ay - 5, 2, 4, r, g, b);
            drawRect(ax + adx*5, ay + 1, 2, 4, r, g, b);
        } else {
            drawRect(ax - 5, ay + ady*5, 4, 2, r, g, b);
            drawRect(ax + 1, ay + ady*5, 4, 2, r, g, b);
        }
    };

    if (!nightVisionMode)
    {
        // Blue square markers above each uncollected energy cell
        for (const auto& cell : energyCells)
        {
            if (cell.collected || cell.phase != PickupPhase::Idle) continue;
            int sx, sy; bool onScreen;
            if (!projectWorld(cell.position + glm::vec3(0.0f, 1.2f, 0.0f), sx, sy, onScreen))
                continue;
            if (onScreen) {
                // Offset down slightly so markers don't overlap the top HUD text
                int msy = glm::max(sy, 52);
                drawFrame(msy == sy ? sx - 10 : sx - 10, msy - 10, 20, 20, 0.22f, 0.88f, 1.00f);
                drawRect (sx - 2, msy - 2, 4, 4, 1.00f, 1.00f, 1.00f);
            } else {
                int dx = (sx == 24) ? -1 : (sx == width - 24) ? 1 : 0;
                int dy = (sy == 24) ? -1 : (sy == height - 24) ? 1 : 0;
                drawArrow(sx, sy, dx, dy, 0.22f, 0.88f, 1.00f);
            }
        }
    }
    else
    {
        // Night-vision mode — white reticles for dust cloud markers
        const std::vector<glm::vec3>& shown = dustClusterPositions;

        // NV status bar top-right
        {
            int bx = width - 170, by = height - 50;
            drawRect (bx, by, 160, 28, 0.02f, 0.08f, 0.02f);
            drawFrame(bx, by, 160, 28, 1.00f, 1.00f, 1.00f);
            drawBitmapText(bx + 6, by + 8,
                "NV  " + std::to_string((int)shown.size()) + " CLOUDS",
                3, 1.00f, 1.00f, 1.00f);
        }

        float pulse = 0.75f + 0.25f * sinf(nowT * 4.0f);

        for (const auto& wp : shown)
        {
            int mx, my; bool onScreen;
            projectWorld(wp + glm::vec3(0.0f, 4.0f, 0.0f), mx, my, onScreen);

            if (onScreen)
            {
                // Large target reticle — two boxes + crosshair ticks
                drawFrame(mx - 24, my - 24, 48, 48, pulse, pulse, pulse);
                drawFrame(mx - 16, my - 16, 32, 32, 0.20f*pulse, 1.00f*pulse, 0.35f*pulse);
                drawRect(mx - 38, my - 2, 14, 4, pulse, pulse, pulse);
                drawRect(mx + 24, my - 2, 14, 4, pulse, pulse, pulse);
                drawRect(mx -  2, my - 38, 4, 14, pulse, pulse, pulse);
                drawRect(mx -  2, my + 24, 4, 14, pulse, pulse, pulse);
                drawRect(mx - 5, my - 5, 10, 10, 1.00f, 1.00f, 1.00f);
                drawBitmapText(mx - 24, my + 28, "DUST", 3, 1.00f, 1.00f, 0.70f);
            }
            else
            {
                // Off-screen — edge arrow pointing toward the cloud
                bool atLeft   = (mx <= 24 + 2);
                bool atRight  = (mx >= width  - 24 - 2);
                bool atBottom = (my <= 24 + 2);
                bool atTop    = (my >= height - 24 - 2);

                // Prefer horizontal over vertical when at a corner
                if (atLeft) {
                    drawRect(mx,      my - 4, 20, 8, pulse, pulse, pulse);
                    drawRect(mx,      my - 12, 8, 24, pulse, pulse, pulse);
                } else if (atRight) {
                    drawRect(mx - 20, my - 4, 20, 8, pulse, pulse, pulse);
                    drawRect(mx - 8,  my - 12, 8, 24, pulse, pulse, pulse);
                } else if (atBottom) {
                    drawRect(mx - 4, my,      8, 20, pulse, pulse, pulse);
                    drawRect(mx - 12, my,     24, 8, pulse, pulse, pulse);
                } else if (atTop) {
                    drawRect(mx - 4, my - 20, 8, 20, pulse, pulse, pulse);
                    drawRect(mx - 12, my - 8, 24, 8, pulse, pulse, pulse);
                } else {
                    drawRect(mx - 8, my - 8, 16, 16, pulse, pulse, pulse);
                }
                drawBitmapText(mx - 12, my + 14, "DUST", 2, 1.00f, 1.00f, 0.70f);
            }
        }
    }

    // Show a HOME marker once all cells are collected
    if (collected >= total && total > 0 && !gameWon)
    {
        const glm::vec3 homeWorld(HOME_PLATFORM_X,
                                  HOME_WIN_Y + 2.5f,   // raise above platform surface
                                  HOME_PLATFORM_Z);
        glm::vec4 hClip = vp * glm::vec4(homeWorld, 1.0f);
        if (hClip.w > 0.0f)
        {
            glm::vec3 hNdc = glm::vec3(hClip) / hClip.w;
            if (fabsf(hNdc.x) <= 1.1f && fabsf(hNdc.y) <= 1.1f)
            {
                int hx = static_cast<int>((hNdc.x * 0.5f + 0.5f) * width);
                int hy = static_cast<int>((hNdc.y * 0.5f + 0.5f) * height);

                float pulse = 0.55f + 0.45f * sinf(nowT * 5.0f);
                drawFrame(hx - 18, hy - 18, 36, 36,
                    0.22f * pulse, 1.00f * pulse, 0.50f * pulse);
                drawRect(hx - 3, hy - 3, 6, 6,
                    0.20f * pulse, 1.00f * pulse, 0.45f * pulse);
                drawBitmapText(hx - 24, hy + 22, "HOME",
                    3, 0.22f * pulse, 1.00f * pulse, 0.55f * pulse);
            }
        }
    }

    // Black hole fall cinematic — plays when the player gets sucked in
    if (fadeState == 6)
    {
        float T   = endStateTimer;
        int   SCX = width / 2;
        int   SCY = height / 2;

        // Fast repeatable random 0..1 from a seed
        auto frand = [](int seed) -> float {
            seed = (seed ^ 61) ^ (seed >> 16);
            seed *= 9301 + seed * 49297;
            return fabsf(fmodf((float)seed * 0.000015259f, 1.0f));
        };

        // Phase 1 (0–5s): systems fail one by one with screen tears and red vignette
        if (T < 5.0f)
        {
            float ramp  = glm::clamp(T / 5.0f, 0.0f, 1.0f);
            float blink = (sinf(T * 22.0f) > 0.0f) ? 1.0f : 0.0f;
            float blink2= (sinf(T * 55.0f) > 0.2f) ? 1.0f : 0.0f;

            // Red vignette grows thicker over time
            int   vW = (int)(ramp * ramp * 80.0f) + 4;
            float vR = 0.55f + ramp * 0.45f;
            drawRect(0,          0,          width, vW,     vR, 0.02f, 0.0f);
            drawRect(0,          height - vW, width, vW,    vR, 0.02f, 0.0f);
            drawRect(0,          0,          vW,    height, vR, 0.02f, 0.0f);
            drawRect(width - vW, 0,          vW,    height, vR, 0.02f, 0.0f);

            // Random horizontal scan-line glitches
            int numTears = (int)(ramp * ramp * 18.0f);
            for (int ti = 0; ti < numTears; ti++)
            {
                int   ty   = (int)(frand(ti * 17 + (int)(T * 7)) * (float)height);
                int   tw   = (int)(frand(ti * 31 + 99) * (float)width * 0.55f) + 40;
                int   tx   = (int)(frand(ti * 43 + (int)(T * 13)) * (float)(width - tw));
                float tb   = 0.25f + frand(ti * 11) * 0.55f;
                drawRect(tx, ty, tw, 2, tb * vR, tb * 0.08f, 0.0f);
            }

            // System failure messages appear briefly then cut off
            struct SysMsg { float t0; const char* msg; int x; int y; };
            SysMsg sysmsgs[] = {
                { 0.4f, "WARNING  HULL STRESS 847 PCT",   SCX - 118, SCY + 68 },
                { 0.9f, "TETHER SEVERED",                 SCX - 64,  SCY + 40 },
                { 1.4f, "NAVIGATION OFFLINE",             SCX - 80,  SCY + 16 },
                { 1.9f, "COMM ARRAY FAILURE",             SCX - 84,  SCY - 12 },
                { 2.5f, "LIFE SUPPORT CRITICAL",          SCX - 96,  SCY - 40 },
                { 3.0f, "STRUCTURAL INTEGRITY  0 PCT",    SCX - 128, SCY - 68 },
                { 3.6f, "ESCAPE VECTOR  IMPOSSIBLE",      SCX - 120, SCY - 96 },
                { 4.2f, "ALL SYSTEMS LOST",               SCX - 76,  SCY - 130},
            };
            for (const SysMsg& m : sysmsgs)
            {
                if (T < m.t0) continue;
                float age  = T - m.t0;
                float fade = glm::clamp(age / 0.25f, 0.0f, 1.0f)
                           * glm::clamp(1.0f - (age - 0.6f) / 0.4f, 0.0f, 1.0f);
                if (fade <= 0.0f) continue;
                float flk  = (sinf(T * 38.0f + m.t0 * 11.0f) > -0.3f) ? 1.0f : 0.40f;
                drawBitmapText(m.x, m.y, m.msg, 3,
                    fade * flk * vR, fade * flk * 0.08f, 0.0f);
            }

            // Final "SIGNAL LOST" message stays and pulses
            if (T > 4.0f)
            {
                float pulse = (blink2 > 0.5f) ? 1.0f : 0.20f;
                drawBitmapText(SCX - 68, SCY + 94, "SIGNAL  LOST",
                    5, pulse * vR, pulse * 0.03f, 0.0f);
            }
        }

        // Phase 2 (5–8s): white flash as you cross the event horizon
        if (T >= 5.0f && T < 8.0f)
        {
            float wordT = glm::clamp((T - 5.5f) / 0.5f, 0.0f, 1.0f)
                        * glm::clamp(1.0f - (T - 6.8f) / 0.8f, 0.0f, 1.0f);
            if (wordT > 0.01f)
            {
                float darkness = 1.0f - wordT * 0.92f;
                drawBitmapText(SCX - 48, SCY + 8,  "FALLING",
                    7, darkness, darkness * 0.18f, 0.0f);
            }
        }

        // Phase 3 (8–22s): amber infinite-corridor bookshelf scene
        if (T >= 8.0f && T < 22.5f)
        {
            float tIn   = glm::clamp((T - 8.0f) / 1.5f, 0.0f, 1.0f);
            float tOut  = glm::clamp(1.0f - (T - 21.0f) / 1.5f, 0.0f, 1.0f);
            float tA    = tIn * tOut;

            drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

            // Perspective shelves drifting toward camera
            float drift     = (T - 8.0f) * 0.045f;
            const int ROWS  = 60;
            const int COLS  = 14;

            for (int fl = 0; fl < ROWS; fl++)
            {
                float raw   = (float)fl / (float)ROWS;
                float depth = fmodf(raw + drift, 1.0f);
                if (depth < 0.0f) depth += 1.0f;
                float zf    = 1.0f / (0.15f + depth * 3.5f);  // perspective scale
                float bright= glm::clamp(zf * 0.80f, 0.0f, 1.0f) * tA;
                if (bright < 0.005f) continue;

                int lW  = (int)((float)width  * zf * 2.2f);
                int lH  = glm::max(1, (int)(zf * 4.0f));
                int lY  = SCY + (int)((depth - 0.5f) * (float)height * zf * 2.6f);
                if (lY < 0 || lY >= height) continue;
                // Warm amber colour — brighter nearby, orange far away
                float warmR = 0.92f - depth * 0.35f;
                float warmG = 0.38f - depth * 0.22f;
                drawRect(SCX - lW / 2, lY, lW, lH, bright * warmR, bright * warmG, 0.0f);

                // Vertical bookshelf columns
                int spacing = glm::max(2, (int)(zf * (float)width * 0.28f));
                int colH    = glm::max(1, (int)(zf * (float)height * 0.28f));
                for (int ci = -COLS/2; ci <= COLS/2; ci++)
                {
                    int cx2 = SCX + ci * spacing;
                    if (cx2 < 0 || cx2 >= width) continue;
                    drawRect(cx2, lY - colH/2, glm::max(1,(int)(zf*2.5f)), colH,
                             bright * 0.50f, bright * 0.18f, 0.0f);

                    // Books between adjacent shelves
                    if (zf > 0.22f)
                    {
                        float bseed = frand(fl * 17 + ci * 5 + 3);
                        if (bseed > 0.35f)
                        {
                            float bseed2 = frand(fl * 31 + ci * 7 + 11);
                            int   bx     = cx2 + (int)(bseed2 * spacing * 0.85f);
                            int   bh2    = glm::max(2, (int)(colH * (0.3f + bseed * 0.5f)));
                            float bb2    = bright * (0.4f + bseed * 0.45f);
                            drawRect(bx, lY + lH + 1,
                                     glm::max(1, (int)(spacing * 0.18f)), bh2,
                                     bb2 * 0.75f, bb2 * 0.25f, 0.0f);
                        }
                    }
                }
            }

            // Concentric rectangles collapsing inward — speeds up as time runs out
            float gridAccel = 1.0f + (T - 8.0f) * 0.18f;
            for (int gr = 0; gr < 12; gr++)
            {
                float phase  = fmodf((float)gr / 12.0f + T * 0.042f * gridAccel, 1.0f);
                float gScale = 1.0f - phase;
                int   gW     = (int)(gScale * (float)width  * 0.88f);
                int   gH     = (int)(gScale * (float)height * 0.88f);
                if (gW < 4 || gH < 4) continue;
                float gb2 = phase * 0.18f * tA;
                // Top / bottom
                drawRect(SCX - gW/2, SCY - gH/2,     gW, 1, gb2 * 0.9f, gb2 * 0.32f, 0.0f);
                drawRect(SCX - gW/2, SCY + gH/2 - 1, gW, 1, gb2 * 0.9f, gb2 * 0.32f, 0.0f);
                // Left / right
                drawRect(SCX - gW/2,     SCY - gH/2, 1, gH, gb2 * 0.9f, gb2 * 0.32f, 0.0f);
                drawRect(SCX + gW/2 - 1, SCY - gH/2, 1, gH, gb2 * 0.9f, gb2 * 0.32f, 0.0f);
            }

            // Singularity glow — slowly builds then surges at the end
            float singT   = glm::clamp((T - 10.0f) / 12.0f, 0.0f, 1.0f);
            float singSize = 30.0f + singT * singT * 180.0f;
            for (int rr = (int)singSize; rr > 0; rr -= 4)
            {
                float rf2 = 1.0f - (float)rr / singSize;
                float sg  = rf2 * rf2 * 0.55f * tA;
                drawRect(SCX - rr, SCY - 1, rr * 2, 3, sg * 1.1f, sg * 0.45f, sg * 0.01f);
                drawRect(SCX - 1, SCY - rr, 3, rr * 2, sg * 1.1f, sg * 0.45f, sg * 0.01f);
            }

            // Amber data streaks falling down, speeding up over time
            float streamSpeed = 55.0f + (T - 8.0f) * 8.0f;
            for (int ds = 0; ds < 30; ds++)
            {
                float sX  = frand(ds * 4273 + 1337) * (float)width;
                float spd = 0.8f + frand(ds * 1619) * 0.6f;
                float sY  = fmodf((T - 8.0f) * spd * streamSpeed + frand(ds * 7919) * (float)height,
                                  (float)height);
                float sb2 = (0.18f + 0.55f * frand(ds * 3141)) * tA;
                int   sh2 = 5 + (ds % 4) * 4;
                drawRect((int)sX, (int)sY, 2, sh2,
                         sb2 * 0.90f, sb2 * 0.32f, 0.0f);
            }

            // Mission echo messages that fade in then burn away
            struct Echo2 {
                float showAt, hideAt;
                int   x, y, sz;
                float r, g, b;
                const char* msg;
            };
            Echo2 echoes[] = {
                {  8.5f, 22.f, SCX-100,  SCY+110, 3, 0.90f,0.32f,0.0f, "MISSION LOG CORRUPTED"     },
                {  9.2f, 22.f, SCX- 80,  SCY+ 82, 3, 0.80f,0.28f,0.0f, "CREW CELLS  LOST"          },
                { 10.0f, 22.f, SCX-112,  SCY+ 54, 3, 0.95f,0.38f,0.0f, "TETHER ANCHOR POINT ZERO"  },
                { 10.8f, 22.f, SCX- 72,  SCY+ 26, 3, 0.70f,0.22f,0.0f, "ALL TIMELINES CONVERGE"    },
                { 11.8f, 22.f, SCX- 88,  SCY-  2, 4, 1.00f,0.50f,0.0f, "YOU REACHED THE EDGE"      },
                { 12.8f, 22.f, SCX-104,  SCY- 34, 3, 0.85f,0.30f,0.0f, "NO SIGNAL ESCAPES"         },
                { 13.8f, 22.f, SCX- 88,  SCY- 66, 3, 0.75f,0.25f,0.0f, "GRAVITY WINS"              },
                { 15.0f, 22.f, SCX- 64,  SCY- 98, 3, 0.60f,0.18f,0.0f, "THE DATA SURVIVES"         },
                { 16.5f, 22.f, SCX-112,  SCY-130, 3, 0.70f,0.22f,0.0f, "BEYOND PLANCKS WALL"       },
                { 18.0f, 22.f, SCX- 96,  SCY+140, 4, 0.98f,0.42f,0.0f, "YOUR SACRIFICE MATTERED"   },
                { 20.0f, 22.f, SCX- 88,  SCY-162, 5, 1.00f,0.55f,0.0f, "REMEMBERED"                },
            };
            for (const Echo2& e : echoes)
            {
                if (T < e.showAt || T >= e.hideAt) continue;
                float age2 = T - e.showAt;
                float ea   = glm::clamp(age2 / 0.6f, 0.0f, 1.0f);
                float burn = glm::clamp(1.0f - (age2 - 2.5f) / 3.0f, 0.0f, 1.0f);
                float flk2 = (sinf(T * 9.0f + e.showAt * 7.0f) > -0.5f) ? 1.0f : 0.45f;
                float vis  = ea * burn * flk2 * tA;
                drawBitmapText(e.x, e.y, e.msg, e.sz,
                    e.r * vis, e.g * vis, e.b * vis);
            }

            // Physics data scrolling up the left side
            if (T > 11.0f)
            {
                float pA = glm::clamp((T - 11.0f) / 1.5f, 0.0f, 1.0f) * tA;
                const char* lines[] = {
                    "SCHWARZSCHILD RADIUS  419 M",
                    "HAWKING TEMP  1.45E-9 K",
                    "TIDAL FORCE  2.4E14 N-M",
                    "DOPPLER SHIFT  Z  8.7",
                    "PHOTON SPHERE  628 M",
                    "TIME DILATION  INFINITE",
                    "SPACETIME CURVATURE  MAX",
                };
                int nlines = 7;
                for (int li = 0; li < nlines; li++)
                {
                    float scrollY = (float)(height - 30 - li * 22)
                                  - (T - 11.0f) * 12.0f;
                    if (scrollY < 0 || scrollY > height) continue;
                    float la = pA * glm::clamp(1.0f - fabsf(scrollY - (float)(height/2)) / (float)(height/2), 0.0f, 1.0f);
                    drawBitmapText(12, (int)scrollY, lines[li], 2,
                        la * 0.60f, la * 0.20f, 0.0f);
                }
            }
        }

        // Phase 4 (22–26s): everything collapses to a single point
        if (T >= 22.0f && T < 26.0f)
        {
            drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

            float colT  = glm::clamp((T - 22.0f) / 4.0f, 0.0f, 1.0f);
            float bloom = glm::clamp(1.0f - (T - 23.5f) / 2.5f, 0.0f, 1.0f);

            // Lines collapse inward
            int   maxR = (int)((1.0f - colT * colT) * (float)glm::min(width, height) * 0.52f);
            for (int rr = maxR; rr > 0; rr -= 3)
            {
                float rf3  = 1.0f - (float)rr / (float)glm::max(maxR, 1);
                float sg3  = rf3 * rf3 * bloom;
                int   th3  = glm::max(1, (int)(sg3 * 4.0f));
                drawRect(SCX - rr, SCY - th3/2, rr*2, th3, sg3*1.1f, sg3*0.40f, 0.0f);
                drawRect(SCX - th3/2, SCY - rr, th3, rr*2, sg3*1.1f, sg3*0.40f, 0.0f);
            }

            // The last visible point of light
            float ptSize = glm::max(0.0f, bloom) * 14.0f;
            if (ptSize > 0.5f)
            {
                int ps = (int)ptSize;
                drawRect(SCX - ps, SCY - ps, ps*2, ps*2, bloom, bloom * 0.50f, bloom * 0.05f);
            }

            // Final message fades in then burns out
            if (T > 22.8f && T < 25.5f)
            {
                float ma = glm::clamp((T - 22.8f) / 0.7f, 0.0f, 1.0f)
                         * glm::clamp(1.0f - (T - 24.0f) / 1.5f, 0.0f, 1.0f);
                float pulse2 = 0.85f + 0.15f * sinf(T * 6.0f);
                drawBitmapText(SCX - 96,  SCY + 28, "ARRAY CONSUMED",
                    5, ma * pulse2, ma * pulse2 * 0.28f, 0.0f);
                drawBitmapText(SCX - 80, SCY - 8, "PILOT LOST",
                    5, ma * pulse2, ma * pulse2 * 0.22f, 0.0f);
                if (T > 24.5f)
                {
                    float ga2 = glm::clamp((T - 24.5f) / 1.0f, 0.0f, 1.0f);
                    drawBitmapText(SCX - 56, SCY - 48, "G  CONTINUE",
                        3, ga2 * 0.45f, ga2 * 0.45f, ga2 * 0.45f);
                }
            }
        }
    }

    // Static loss screen shown after the cinematic ends
    if (gameLost && fadeState == 7)
    {
        float Ts = (float)glfwGetTime();
        int   SCX2 = width / 2, SCY2 = height / 2;
        drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

        // Twinkling starfield background
        for (int si = 0; si < 280; ++si) {
            float sx   = fmodf((float)(si * 4273 + 1337), (float)width);
            float sy   = fmodf((float)(si * 7919 + 2311), (float)height);
            float sbr  = 0.06f + 0.20f * fmodf((float)(si * 1619), 1.0f);
            float twk  = 0.80f + 0.20f * sinf(Ts * (0.8f + fmodf((float)(si * 0.19f), 1.2f)) + (float)si);
            sbr *= twk;
            int sw2 = (si % 11 == 0) ? 2 : 1;
                drawRect((int)sx, (int)sy, sw2, sw2, sbr * 0.88f, sbr * 0.90f, sbr);
        }

        // Slowly rotating photon ring
        float phRot = Ts * 0.04f;
        for (int ri = 0; ri < 360; ri += 3)
        {
            float ang = (float)ri * 3.14159f / 180.0f + phRot;
            float dop = 0.30f + 0.70f * glm::clamp(sinf(ang * 2.0f), 0.0f, 1.0f);
            int   rx  = SCX2 + (int)(cosf(ang) * 140.0f);
            int   ry  = SCY2 + (int)(sinf(ang) *  48.0f);  // ellipse — seen at angle
            float rb3 = dop * (0.15f + 0.22f * sinf(ang * 8.0f + Ts * 0.5f));
            drawRect(rx - 1, ry - 1, 3, 3, rb3 * 0.9f, rb3 * 0.28f, 0.0f);
        }

        // Black hole shadow at centre
        int shadowR = 88;
        drawRect(SCX2 - shadowR, SCY2 - shadowR / 3,
                 shadowR * 2, (shadowR * 2) / 3, 0.0f, 0.0f, 0.0f);

        // Inner glow around the shadow
        for (int ri = shadowR + 20; ri > shadowR - 4; ri -= 2)
        {
            float rf4  = 1.0f - (float)(ri - shadowR + 4) / 24.0f;
            float rg4  = rf4 * rf4 * 0.28f;
            drawRect(SCX2 - ri, SCY2 - 1, ri * 2, 3, rg4 * 0.9f, rg4 * 0.28f, 0.0f);
        }

        // Loss screen text
        float pulse3 = 0.88f + 0.12f * sinf(Ts * 0.9f);

        drawBitmapText(SCX2 - 132, SCY2 + 128,
            "TON 618", 5, 0.45f, 0.45f, 0.45f);

        drawBitmapText(SCX2 - 148, SCY2 + 94,
            "TETHER PILOT  LOST TO SINGULARITY",
            3, 0.38f * pulse3, 0.38f * pulse3, 0.38f * pulse3);

        drawRect(SCX2 - 240, SCY2 + 80, 480, 1, 0.25f, 0.10f, 0.0f);

        drawBitmapText(SCX2 - 110, SCY2 + 62,
            "MASS CONTRIBUTION DETECTED",
            3, 0.55f, 0.12f, 0.0f);
        drawBitmapText(SCX2 - 80, SCY2 + 40,
            "THE SINGULARITY IS GROWING",
            3, 0.40f * pulse3, 0.08f * pulse3, 0.0f);

        // Final score display
        std::string scoreEpitaph = "FINAL SCORE  " + std::to_string(playerScore);
        drawBitmapText(SCX2 - (int)(scoreEpitaph.size() * 6),
                       SCY2 + 10, scoreEpitaph,
                       4, 0.60f * pulse3, 0.22f * pulse3, 0.0f);

        // Divider line
        drawRect(SCX2 - 240, SCY2 - 4, 480, 1, 0.25f, 0.10f, 0.0f);

        drawBitmapText(SCX2 - 44, SCY2 - 28,
            "G  TRY AGAIN", 3, 0.40f, 0.40f, 0.40f);
    }

    // Win cutscene — hyperspace streaks, planet flybys, then Earth
    if (fadeState == 4)
    {
        float T = endStateTimer;
        int   SCX = width / 2;
        int   SCY = height / 2;

        // Black background
        drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

        // Starfield appears after the initial streak phase
        if (T >= 2.5f)
        {
            for (int si = 0; si < 220; ++si)
            {
                float sx = fmodf((float)(si * 4273 + 1337), (float)width);
                float sy = fmodf((float)(si * 7919 + 2311), (float)height);
                float sb = 0.25f + 0.60f * fmodf((float)(si * 1619), 1.0f);
                float twinkle = 0.82f + 0.18f * sinf(T * (1.5f + fmodf(si * 0.19f, 1.4f)) + si);
                sb *= twinkle;
                int sw = (si % 7 == 0) ? 3 : ((si % 4 == 0) ? 2 : 1);
                drawRect((int)sx, (int)sy, sw, sw, sb * 0.85f, sb * 0.90f, sb);
            }
        }

        // Phase A (0–2.5s): hyperspace streaks radiating out
        if (T < 2.5f)
        {
            float st = glm::clamp(T / 2.5f, 0.0f, 1.0f);
            float st2 = st * st;
            for (int s2 = 0; s2 < 40; ++s2)
            {
                float ang = (float)s2 / 40.0f * 6.28318f;
                float lenMul = 0.40f + 0.60f * fabsf(sinf(ang * 7.0f + 1.1f));
                float len = st2 * (float)width * 0.78f * lenMul;
                if (len < 2.0f) continue;
                int steps = glm::max(2, (int)(len / 4.0f));
                for (int step = 0; step < steps; ++step)
                {
                    float frac = (float)step / steps;
                    int   px = SCX + (int)(cosf(ang) * frac * len);
                    int   py = SCY + (int)(sinf(ang) * frac * len);
                    float bright = st * (1.0f - frac * 0.52f);
                    drawRect(px - 1, py - 1, 3, 3, bright * 0.82f, bright * 0.90f, bright);
                }
            }
        }

        // Helper: draw a shaded planet disc
        auto drawPlanet = [&](int cx, int cy, int r,
            float dkR, float dkG, float dkB,
            float ltR, float ltG, float ltB,
            float alpha)
            {
                if (r < 2 || alpha < 0.01f) return;
                for (int row = -r; row <= r; row += 4)
                {
                    float rf = (float)row / r;
                    int   hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * r);
                    if (hw < 1) continue;
                    float band = 0.80f + 0.20f * sinf(rf * 12.0f);
                    for (int col = -hw; col <= hw; col += 4)
                    {
                        float dx = (float)col / r, dy = rf;
                        float ndotl = glm::clamp(-0.55f * dx - 0.72f * dy + 0.58f, 0.0f, 1.0f);
                        float term = glm::smoothstep(0.0f, 0.28f, ndotl);
                        float sh = ndotl * band * term;
                        drawRect(cx + col - 2, cy + row - 2, 4, 4,
                            glm::mix(dkR * 0.10f, ltR, sh) * alpha,
                            glm::mix(dkG * 0.10f, ltG, sh) * alpha,
                            glm::mix(dkB * 0.10f, ltB, sh) * alpha);
                    }
                }
            };

        // Helper: draw atmosphere glow around a planet
        auto drawAtmos = [&](int cx, int cy, int r,
            float ar, float ag, float ab, float alpha)
            {
                if (r < 2 || alpha < 0.01f) return;
                int ar2 = (int)(r * 1.10f);
                for (int row = -ar2; row <= ar2; row += 4)
                {
                    float rf = (float)row / ar2;
                    int   hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * ar2);
                    int   hw2 = (int)(sqrtf(glm::max(0.0f, 1.0f - (float)(row * row) /
                        (float)(r * r))) * r);
                    int   ow = glm::max(0, hw - glm::max(0, hw2));
                    if (ow < 1) continue;
                    float ha = (1.0f - fabsf(rf)) * alpha * 0.55f;
                    drawRect(cx - hw, cy + row, ow, 4, ar * ha, ag * ha, ab * ha);
                    drawRect(cx + hw2, cy + row, ow, 4, ar * ha, ag * ha, ab * ha);
                }
            };

        // Phase B (5.2–12s): four planets fly past
        struct FlybyDef {
            float startT, dur;
            float exFrac, eyFrac, exitXFrac, exitYFrac;
            int peakR;
            float dR, dG, dB, lR, lG, lB, aR, aG, aB;
        };
        // Planet flyby definitions: startTime, duration, entry/exit positions, peak radius, colours
        FlybyDef flybys[4] = {
            { 5.2f, 1.8f,  1.20f,-0.35f,-0.30f,1.10f, 58,   // rocky grey-brown
              0.30f,0.24f,0.18f, 0.65f,0.52f,0.36f, 0.55f,0.68f,0.85f },
            { 7.2f, 1.5f,  1.15f, 0.10f,-0.20f,0.60f, 42,   // red volcanic
              0.40f,0.08f,0.04f, 0.85f,0.28f,0.08f, 0.90f,0.48f,0.22f },
            { 8.8f, 2.2f,  0.60f,-1.20f, 0.30f,1.15f, 80,   // icy-blue gas giant
              0.12f,0.18f,0.40f, 0.28f,0.55f,0.92f, 0.35f,0.68f,1.00f },
            {10.5f, 1.6f,  1.20f, 0.80f,-0.15f,-0.20f,36,   // dark purple rocky
              0.22f,0.10f,0.30f, 0.48f,0.22f,0.62f, 0.58f,0.38f,0.85f },
        };

        for (int fi = 0; fi < 4; ++fi)
        {
            const FlybyDef& fb = flybys[fi];
            if (T < fb.startT || T > fb.startT + fb.dur) continue;
            float ft = (T - fb.startT) / fb.dur;
            int   rx = (int)(SCX + glm::mix(fb.exFrac, fb.exitXFrac, ft) * width * 0.5f);
            int   ry = (int)(SCY + glm::mix(fb.eyFrac, fb.exitYFrac, ft) * height * 0.5f);
            float pk = 1.0f - fabsf(ft - 0.45f) / 0.55f;
            pk = glm::clamp(pk, 0.0f, 1.0f)
                * glm::smoothstep(0.0f, 0.12f, ft)
                * glm::smoothstep(1.0f, 0.88f, ft);
            int r2 = (int)(fb.peakR * pk);
            drawAtmos(rx, ry, r2, fb.aR, fb.aG, fb.aB, pk);
            drawPlanet(rx, ry, r2, fb.dR, fb.dG, fb.dB, fb.lR, fb.lG, fb.lB, pk);
        }

        // Phase C (12–18s): Earth grows to fill screen
        if (T >= 12.0f)
        {
            float et = glm::clamp((T - 12.0f) / 6.0f, 0.0f, 1.0f);
            float etE = et * et * (3.0f - 2.0f * et);

            int ECX = SCX + (int)(width * 0.08f);
            int ECY = -(int)((float)height * 0.05f);
            int ER = (int)(6.0f + etE * (float)height * 1.10f);
            if (ER < 2) ER = 2;

            // Draw the Earth
            drawEarthHelper(ECX, ECY, ER, etE, drawRect);
        }

        // Phase D (15.5–18s): win panel fades in over Earth
        if (T >= 15.5f)
        {
            float pa = glm::clamp((T - 15.5f) / 2.5f, 0.0f, 1.0f);
            float ta = glm::clamp((T - 16.0f) / 2.0f, 0.0f, 1.0f);
            int PW = 600, PH = 260, PX = SCX - PW / 2, PY = (int)(height * 0.36f);
            drawRect(PX, PY, PW, PH, 0.00f * pa, 0.02f * pa, 0.06f * pa);
            drawFrame(PX, PY, PW, PH, 0.18f * pa, 0.88f * pa, 0.50f * pa);
            if (ta > 0.01f) {
                drawBitmapText(SCX - 160, PY + PH - 30, "ARRAY STABILISED", 6, 0.22f * ta, 1.00f * ta, 0.55f * ta);
                drawBitmapText(SCX - 128, PY + PH - 68, "ALL ENERGY CELLS RECOVERED", 3, 0.68f * ta, 0.95f * ta, 1.00f * ta);
                int wm = (int)(missionFinalTime / 60.0f), ws = (int)(missionFinalTime) % 60;
                std::ostringstream ts;
                ts << "MISSION TIME  " << std::setw(2) << std::setfill('0') << wm << ":"
                    << std::setw(2) << std::setfill('0') << ws;
                drawBitmapText(SCX - 100, PY + PH - 100, ts.str(), 3, 0.80f * ta, 0.80f * ta, 0.80f * ta);
                std::string sc = "FINAL SCORE  " + std::to_string(playerScore);
                drawBitmapText(SCX - (int)(sc.size() * 5), PY + PH - 128, sc, 4, 1.00f * ta, 0.78f * ta, 0.30f * ta);

                // Revelation — staggered fade-in after 18s
                float rev1 = glm::clamp((T - 18.2f) / 1.2f, 0.0f, 1.0f) * ta;
                float rev2 = glm::clamp((T - 19.8f) / 1.2f, 0.0f, 1.0f) * ta;
                float rev3 = glm::clamp((T - 21.5f) / 1.0f, 0.0f, 1.0f) * ta;
                if (rev1 > 0.01f) {
                    float rp = 0.85f + 0.15f * sinf(T * 2.2f);
                    drawBitmapText(SCX - 148, PY + PH - 165, "ENERGY OUTPUT  INCREASING?",
                        3, rev1 * rp * 0.90f, rev1 * rp * 0.40f, rev1 * rp * 0.05f);
                }
                if (rev2 > 0.01f)
                    drawBitmapText(SCX - 100, PY + PH - 188, "THAT IS NOT POSSIBLE.",
                        3, rev2 * 0.70f, rev2 * 0.15f, 0.0f);
                if (rev3 > 0.01f) {
                    // Divider then the final twist
                    drawRect(PX + 20, PY + PH - 207, PW - 40, 1, 0.35f * rev3, 0.08f * rev3, 0.0f);
                    drawBitmapText(SCX - 144, PY + PH - 224, "THE ARRAY WAS NEVER HARVESTING.",
                        2, rev3 * 0.55f, rev3 * 0.55f, rev3 * 0.55f);
                    drawBitmapText(SCX - 132, PY + PH - 240, "IT WAS BUILDING SOMETHING.",
                        2, rev3 * 0.55f, rev3 * 0.55f, rev3 * 0.55f);
                }

                // "G MAIN MENU" — only show after revelation plays out
                float ga3 = glm::clamp((T - 22.5f) / 1.0f, 0.0f, 1.0f) * ta;
                if (ga3 > 0.01f)
                    drawBitmapText(SCX - 60, PY + 12, "G  MAIN MENU", 3,
                        0.36f * ga3, 0.58f * ga3, 0.36f * ga3);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // STATIC WIN SCREEN  fadeState == 5
    // ═══════════════════════════════════════════════════════════════════════════
    if (gameWon && fadeState == 5)
    {
        int SCX = width / 2, ECX = SCX + (int)(width * 0.08f), ECY = -(int)((float)height * 0.05f);
        int ER = (int)((float)height * 1.10f);
        drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

        // Stars behind Earth
        for (int si = 0; si < 220; ++si) {
            float sx = fmodf((float)(si * 4273 + 1337), (float)width);
            float sy = fmodf((float)(si * 7919 + 2311), (float)height);
            float sb = 0.25f + 0.60f * fmodf((float)(si * 1619), 1.0f);
            int sw = (si % 7 == 0) ? 3 : ((si % 4 == 0) ? 2 : 1);
            drawRect((int)sx, (int)sy, sw, sw, sb * 0.85f, sb * 0.90f, sb);
        }

        // Full photorealistic Earth
        drawEarthHelper(ECX, ECY, ER, 1.0f, drawRect);

        // Win panel
        int PW = 600, PH = 220, PX = SCX - PW / 2, PY = (int)(height * 0.38f);
        drawRect(PX, PY, PW, PH, 0.00f, 0.02f, 0.06f);
        drawFrame(PX, PY, PW, PH, 0.18f, 0.88f, 0.50f);
        drawBitmapText(SCX - 160, PY + PH - 30, "ARRAY STABILISED", 6, 0.22f, 1.00f, 0.55f);
        drawBitmapText(SCX - 128, PY + PH - 68, "ALL ENERGY CELLS RECOVERED", 3, 0.68f, 0.95f, 1.00f);
        int m2 = (int)(missionFinalTime / 60.0f), s2 = (int)(missionFinalTime) % 60;
        std::ostringstream ts2;
        ts2 << "MISSION TIME  " << std::setw(2) << std::setfill('0') << m2 << ":"
            << std::setw(2) << std::setfill('0') << s2;
        drawBitmapText(SCX - 100, PY + PH - 100, ts2.str(), 3, 0.80f, 0.80f, 0.80f);
        std::string sc2 = "FINAL SCORE  " + std::to_string(playerScore);
        drawBitmapText(SCX - (int)(sc2.size() * 5), PY + PH - 128, sc2, 4, 1.00f, 0.78f, 0.30f);
        // Revelation text always visible on static screen
        drawBitmapText(SCX - 148, PY + PH - 165, "ENERGY OUTPUT  INCREASING?",
            3, 0.90f, 0.40f, 0.05f);
        drawBitmapText(SCX - 100, PY + PH - 188, "THAT IS NOT POSSIBLE.",
            3, 0.70f, 0.15f, 0.0f);
        drawRect(PX + 20, PY + PH - 207, PW - 40, 1, 0.35f, 0.08f, 0.0f);
        drawBitmapText(SCX - 144, PY + PH - 222, "THE ARRAY WAS NEVER HARVESTING.",
            2, 0.55f, 0.55f, 0.55f);
        drawBitmapText(SCX - 132, PY + PH - 238, "IT WAS BUILDING SOMETHING.",
            2, 0.55f, 0.55f, 0.55f);
        drawBitmapText(SCX - 60, PY + 14, "G  MAIN MENU", 3, 0.36f, 0.58f, 0.36f);
    }

    if (minigameState == TetherMinigameState::Active)
        drawTetherMinigameUI();

    glDisable(GL_SCISSOR_TEST);
    lensingDebug.draw(width, height);
    glEnable(GL_DEPTH_TEST);
}

// High score file reading and writing
#pragma warning(push)
#pragma warning(disable: 4996)   // suppress MSVC fopen warning
