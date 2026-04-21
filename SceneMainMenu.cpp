#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#pragma warning(push)
#pragma warning(disable: 4996)

void SceneBasic_Uniform::drawLoadingScreen()
{
    auto drawRect = [&](int x, int y, int w, int h, float r, float g, float b) {
        if (w <= 0 || h <= 0) return;
        glScissor(x, y, w, h); glClearColor(r, g, b, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    };
    auto drawBitmapChar = [&](int x, int y, char c, int s, float r, float g, float b) {
        if (c == ' ') return;
        const char* glyph = getBitmapGlyph((char)toupper((unsigned char)c));
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col)
                if (bitmapGlyphPixel(glyph, row, col))
                    drawRect(x + col * s, y + (4 - row) * s, s, s, r, g, b);
    };
    auto drawText = [&](int x, int y, const std::string& txt, int s, float r, float g, float b) {
        int pen = x;
        for (char c : txt) { drawBitmapChar(pen, y, c, s, r, g, b); pen += (c == ' ') ? (2*s) : (4*s); }
    };

    // Black background
    drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

    // Star field (same as menu)
    for (int si = 0; si < 120; ++si) {
        float sx = fmodf((float)(si * 4273 + 1337), (float)width);
        float sy = fmodf((float)(si * 7919 + 2311), (float)height);
        float sb = 0.12f + 0.25f * fmodf((float)(si * 1619), 1.0f);
        drawRect((int)sx, (int)sy, 1, 1, sb * 0.8f, sb * 0.88f, sb);
    }

    int SCX = width / 2, SCY = height / 2;

    // Title
    drawText(SCX - 140, SCY + (int)(height * 0.24f), "TON 618", 5,
        0.22f, 1.00f, 0.55f);
    drawText(SCX - 80, SCY + (int)(height * 0.19f), "STELLAR EXPEDITION", 3,
        0.40f, 0.70f, 0.90f);

    // "INITIALISING SYSTEMS..."  blinking dots
    int dotCount = 1 + (int)(loadingProgress * 3.0f) % 4;
    std::string initMsg = "INITIALISING SYSTEMS";
    for (int d = 0; d < dotCount; ++d) initMsg += ".";
    drawText(SCX - 100, SCY + 20, initMsg, 3, 0.40f, 0.80f, 0.55f);

    // Loading messages that appear at thresholds
    struct { float thr; const char* msg; } steps[] = {
        { 0.10f, "LOADING SHADERS" },
        { 0.25f, "BUILDING MESH GEOMETRY" },
        { 0.42f, "CALIBRATING GRAVITY LENS" },
        { 0.60f, "SCANNING ASTEROID BELT" },
        { 0.75f, "PRIMING ENERGY COLLECTORS" },
    };
    int numSteps = (int)(sizeof(steps) / sizeof(steps[0]));
    int msgY = SCY - 20;
    for (int i = 0; i < numSteps; ++i) {
        if (loadingProgress >= steps[i].thr) {
            float age = loadingProgress - steps[i].thr;
            float br = glm::clamp(age * 10.0f, 0.0f, 1.0f);
            drawText(SCX - 130, msgY - i * 20, steps[i].msg, 2,
                0.20f * br, 0.70f * br, 0.40f * br);
        }
    }

    // ── Progress bar track ────────────────────────────────────────────────────
    int barW = (int)(width * 0.55f);
    int barH = 14;
    int barX = SCX - barW / 2;
    int barY = SCY - (int)(height * 0.22f);

    // Track background
    drawRect(barX - 2, barY - 2, barW + 4, barH + 4, 0.08f, 0.12f, 0.10f);
    // Track fill
    int fillW = (int)(barW * loadingProgress);
    if (fillW > 0) {
        float pulse = 0.75f + 0.25f * sinf(menuTime * 6.0f);
        drawRect(barX, barY, fillW, barH, 0.10f * pulse, 0.60f * pulse, 0.32f * pulse);
    }
    // Track border
    drawRect(barX - 2, barY - 2, barW + 4, 2, 0.20f, 0.55f, 0.30f);
    drawRect(barX - 2, barY + barH, barW + 4, 2, 0.20f, 0.55f, 0.30f);
    drawRect(barX - 2, barY - 2, 2, barH + 4, 0.20f, 0.55f, 0.30f);
    drawRect(barX + barW + 2, barY - 2, 2, barH + 4, 0.20f, 0.55f, 0.30f);

    // ── Spaceship icon riding the bar ─────────────────────────────────────────
    // Ship: a small arrow pointing right
    int shipX = barX + fillW - 4;
    int shipY = barY + barH / 2 - 6;
    // Fuselage
    drawRect(shipX,      shipY + 4,  14, 4, 0.80f, 0.85f, 1.00f);
    // Nose (triangle tip)
    drawRect(shipX + 14, shipY + 5,  2,  2,  0.80f, 0.85f, 1.00f);
    drawRect(shipX + 16, shipY + 6,  2,  1,  0.80f, 0.85f, 1.00f);
    // Wing top
    drawRect(shipX + 2,  shipY + 8,  8,  3,  0.50f, 0.60f, 0.80f);
    // Wing bottom
    drawRect(shipX + 2,  shipY + 1,  8,  3,  0.50f, 0.60f, 0.80f);
    // Engine glow (flickers)
    float glow = 0.6f + 0.4f * sinf(menuTime * 18.0f);
    drawRect(shipX - 4,  shipY + 3,  4,  2,  glow, glow * 0.45f, 0.0f);
    drawRect(shipX - 6,  shipY + 4,  3,  4,  glow * 0.7f, glow * 0.20f, 0.0f);

    // Percent text
    int pct = (int)(loadingProgress * 100.0f);
    drawText(SCX - 14, barY - 22, std::to_string(pct) + " PCT", 2, 0.30f, 0.65f, 0.40f);
}

// ─── drawMainMenu ────────────────────────────────────────────────────────────
void SceneBasic_Uniform::drawMainMenu()
{
    auto drawRect = [&](int x, int y, int w, int h, float r, float g, float b)
        {
            if (w <= 0 || h <= 0) return;
            glScissor(x, y, w, h);
            glClearColor(r, g, b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        };
    auto drawFrame = [&](int x, int y, int w, int h, float r, float g, float b) {
        drawRect(x, y, w, 2, r, g, b); drawRect(x, y + h - 2, w, 2, r, g, b);
        drawRect(x, y, 2, h, r, g, b); drawRect(x + w - 2, y, 2, h, r, g, b);
        };
    auto drawBitmapChar = [&](int x, int y, char c, int s, float r, float g, float b) {
        if (c == ' ')return;
        const char* glyph = getBitmapGlyph((char)toupper((unsigned char)c));
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col)
                if (bitmapGlyphPixel(glyph, row, col))
                    drawRect(x + col * s, y + (4 - row) * s, s, s, r, g, b);
        };
    auto drawText = [&](int x, int y, const std::string& txt, int s, float r, float g, float b) {
        int pen = x;
        for (char c : txt) { drawBitmapChar(pen, y, c, s, r, g, b); pen += (c == ' ') ? (2 * s) : (4 * s); }
        };

    int SCX = width / 2;
    int SCY = height / 2;

    // ── Black background ──────────────────────────────────────────────────────
    drawRect(0, 0, width, height, 0.0f, 0.0f, 0.0f);

    // ── Star field ────────────────────────────────────────────────────────────
    for (int si = 0; si < 180; ++si) {
        float sx = fmodf((float)(si * 4273 + 1337), (float)width);
        float sy = fmodf((float)(si * 7919 + 2311), (float)height);
        float sb = 0.18f + 0.45f * fmodf((float)(si * 1619), 1.0f);
        float tw = 0.82f + 0.18f * sinf(menuTime * (1.2f + fmodf(si * 0.21f, 1.5f)) + si);
        sb *= tw;
        drawRect((int)sx, (int)sy, (si % 5 == 0) ? 2 : 1, (si % 5 == 0) ? 2 : 1, sb * 0.82f, sb * 0.88f, sb);
    }

    // ── Animated black hole graphic (left side) ───────────────────────────────
    int bhCX = (int)(width * 0.28f);
    int bhCY = SCY;
    int bhRG = (int)(glm::min(width, height) * 0.18f);

    for (int row = -bhRG; row <= bhRG; row += 3) {
        float rf = (float)row / bhRG;
        int   hw  = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * bhRG * 1.60f);
        int   hw2 = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * bhRG * 0.90f);
        int   ow  = glm::max(0, hw - hw2);
        if (ow < 1) continue;
        float ang = atan2f(rf, 1.0f) + menuBHAngle;
        float dop = 0.50f + 0.50f * sinf(ang * 2.0f);
        float rr = glm::mix(0.35f, 1.00f, dop) * 0.90f;
        float gg = glm::mix(0.08f, 0.55f, dop) * 0.60f;
        drawRect(bhCX - hw,  bhCY + row, ow, 3, rr, gg, 0.02f);
        drawRect(bhCX + hw2, bhCY + row, ow, 3, rr, gg, 0.02f);
    }
    for (int row = -bhRG; row <= bhRG; row += 3) {
        float rf = (float)row / bhRG;
        int hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * bhRG * 0.88f);
        if (hw < 1) continue;
        drawRect(bhCX - hw, bhCY + row, hw * 2, 3, 0.0f, 0.0f, 0.0f);
    }
    for (int row = -bhRG; row <= bhRG; row += 2) {
        float rf = (float)row / bhRG;
        float R  = bhRG * 0.90f;
        int   hw = (int)(sqrtf(glm::max(0.0f, 1.0f - rf * rf)) * R);
        float rg = 0.82f + 0.18f * sinf(menuBHAngle * 3.0f + rf * 6.0f);
        drawRect(bhCX - hw - 2, bhCY + row, 2, 2, rg, rg * 0.55f, 0.03f);
        drawRect(bhCX + hw,     bhCY + row, 2, 2, rg, rg * 0.55f, 0.03f);
    }

    // ── Title (always visible) ────────────────────────────────────────────────
    int titleX = (int)(width * 0.46f);
    int titleY = SCY + (int)(height * 0.28f);
    drawText(titleX - 20, titleY,       "TON 618", 8, 0.22f, 1.00f, 0.55f);
    drawText(titleX + 28, titleY - 56,  "BlackHole Mission",    5, 0.80f, 0.28f, 0.10f);
    float pulse = 0.75f + 0.25f * sinf(menuTime * 2.2f);
    drawText(titleX + 8, titleY - 96, "ITS A STELLAR EXPEDITION", 4,
        0.55f * pulse, 0.85f * pulse, 1.00f * pulse);
    // Tagline
    float tpulse = 0.55f + 0.45f * sinf(menuTime * 1.1f);
    drawText(titleX + 14, titleY - 124, "Nothing escapes. Not even you.", 2,
        0.60f * tpulse, 0.22f * tpulse, 0.05f * tpulse);

    float nh = 0.55f + 0.45f * sinf(menuTime * 1.8f);

    // ═══════════════════════════════════════════════════════════════════════════
    if (menuPage == 0)
    {
        // ── Main menu items: PLAY / LEADERBOARD / SETTINGS / EXIT ────────────
        int menuX      = titleX + 20;
        int menuStartY = SCY - (int)(height * 0.02f);
        int menuSpacing = 48;

        const char* items[4]  = { "PLAY", "LEADERBOARD", "SETTINGS", "EXIT" };
        float itemR[4] = { 0.22f, 0.30f, 0.55f, 0.80f };
        float itemG[4] = { 1.00f, 0.75f, 0.90f, 0.22f };
        float itemB[4] = { 0.55f, 1.00f, 1.00f, 0.22f };

        for (int i = 0; i < 4; ++i)
        {
            int iy  = menuStartY - i * menuSpacing;
            bool sel = (menuSelection == i);
            if (sel) {
                float sp = 0.70f + 0.30f * sinf(menuTime * 3.5f);
                drawRect (menuX - 8, iy - 4, 280, 42, 0.02f, 0.06f, 0.10f);
                drawFrame(menuX - 8, iy - 4, 280, 42, itemR[i]*sp, itemG[i]*sp, itemB[i]*sp);
                drawRect (menuX - 28, iy + 12, 14, 4, itemR[i]*sp, itemG[i]*sp, itemB[i]*sp);
                drawRect (menuX - 20, iy + 8,  4, 12, itemR[i]*sp, itemG[i]*sp, itemB[i]*sp);
            }
            drawText(menuX, iy + 8, items[i], sel ? 5 : 4,
                sel ? itemR[i] : itemR[i] * 0.45f,
                sel ? itemG[i] : itemG[i] * 0.45f,
                sel ? itemB[i] : itemB[i] * 0.45f);
        }
        drawText(SCX - 110, 18, "W/S SELECT   ENTER CONFIRM   ESC QUIT", 2,
            0.40f * nh, 0.70f * nh, 0.40f * nh);

        // Show top-3 preview beside menu
        if (!highScores.empty()) {
            int hsX = menuX + 310, hsY = menuStartY;
            drawText(hsX, hsY + 8, "TOP SCORES", 3, 0.60f, 0.90f, 0.60f);
            int show = glm::min((int)highScores.size(), 3);
            for (int r = 0; r < show; ++r) {
                std::string line = std::to_string(r + 1) + " " + std::to_string(highScores[r].first);
                drawText(hsX + 8, hsY - 26 - r * 22, line, 2, 0.80f, 0.90f, 0.55f);
            }
        }
    }
    // ═══════════════════════════════════════════════════════════════════════════
    else if (menuPage == 1)
    {
        // ── Leaderboard page ─────────────────────────────────────────────────
        int panX = SCX - 200, panY = SCY - 200, panW = 420, panH = 380;
        drawRect (panX, panY, panW, panH, 0.02f, 0.04f, 0.08f);
        drawFrame(panX, panY, panW, panH, 0.22f, 0.70f, 0.90f);
        drawText(panX + 14, panY + panH - 28, "LEADERBOARD", 5, 0.22f, 0.70f, 1.00f);

        // Column headers
        drawText(panX + 14, panY + panH - 60, "RANK", 2, 0.45f, 0.55f, 0.70f);
        drawText(panX + 90, panY + panH - 60, "SCORE", 2, 0.45f, 0.55f, 0.70f);
        drawText(panX + 220, panY + panH - 60, "TIME", 2, 0.45f, 0.55f, 0.70f);
        // Divider
        drawRect(panX + 10, panY + panH - 66, panW - 20, 2, 0.25f, 0.40f, 0.55f);

        if (highScores.empty()) {
            drawText(panX + 60, panY + panH - 120, "NO SCORES YET", 3, 0.40f, 0.50f, 0.55f);
            drawText(panX + 40, panY + panH - 155, "COMPLETE A MISSION", 2, 0.30f, 0.40f, 0.45f);
        } else {
            int show = glm::min((int)highScores.size(), 10);
            for (int r = 0; r < show; ++r) {
                int rowY = panY + panH - 90 - r * 26;
                // Rank colour: gold/silver/bronze for top 3
                float rr = (r == 0) ? 1.0f : (r == 1) ? 0.75f : (r == 2) ? 0.80f : 0.55f;
                float rg = (r == 0) ? 0.84f : (r == 1) ? 0.75f : (r == 2) ? 0.50f : 0.55f;
                float rb = (r == 0) ? 0.0f  : (r == 1) ? 0.75f : (r == 2) ? 0.10f : 0.55f;
                drawText(panX + 14, rowY, std::to_string(r + 1), 3, rr, rg, rb);
                drawText(panX + 90, rowY, std::to_string(highScores[r].first), 3, rr, rg, rb);
                int mm = (int)(highScores[r].second / 60.0f);
                int ss = (int)(highScores[r].second) % 60;
                std::ostringstream ts; ts << std::setfill('0') << std::setw(2) << mm << ":"
                                         << std::setfill('0') << std::setw(2) << ss;
                drawText(panX + 220, rowY, ts.str(), 3, rr * 0.8f, rg * 0.8f, rb + 0.3f);
            }
        }
        drawText(panX + 60, panY + 12, "ENTER / ESC   BACK", 2, 0.40f * nh, 0.65f * nh, 0.40f * nh);
    }
    // ═══════════════════════════════════════════════════════════════════════════
    else if (menuPage == 2)
    {
        // ── Settings page ─────────────────────────────────────────────────────
        int panX = SCX - 230, panY = SCY - 230, panW = 480, panH = 440;
        drawRect (panX, panY, panW, panH, 0.02f, 0.04f, 0.08f);
        drawFrame(panX, panY, panW, panH, 0.55f, 0.70f, 1.00f);
        drawText(panX + 14, panY + panH - 28, "SETTINGS", 5, 0.55f, 0.75f, 1.00f);
        drawRect(panX + 10, panY + panH - 34, panW - 20, 2, 0.30f, 0.40f, 0.65f);

        // Settings rows: 5 items
        struct SettRow { const char* lbl; float val; float mn; float mx; bool isAction; };
        SettRow rows[5] = {
            { "MUSIC VOL",    audio.musicVol,    0.0f, 1.0f, false },
            { "SFX VOL",      audio.sfxVol,      0.0f, 1.0f, false },
            { "SENSITIVITY",  mouseSensitivity,  0.2f, 3.0f, false },
            { "CONTROLS",     0.0f,              0.0f, 0.0f, true  },
            { "BACK",         0.0f,              0.0f, 0.0f, true  },
        };

        int rowStartY = panY + panH - 70;
        int rowH = 50;
        for (int i = 0; i < 5; ++i)
        {
            int rowY  = rowStartY - i * rowH;
            bool sel  = (settingsSelection == i);
            float br  = sel ? (0.70f + 0.30f * sinf(menuTime * 3.5f)) : 0.45f;
            float lr  = sel ? 0.55f * br : 0.35f;
            float lg  = sel ? 0.85f * br : 0.50f;
            float lb  = sel ? 1.00f * br : 0.65f;

            if (sel) {
                drawRect (panX + 8,  rowY - 4, panW - 16, rowH - 4, 0.03f, 0.06f, 0.12f);
                drawFrame(panX + 8,  rowY - 4, panW - 16, rowH - 4, lr, lg, lb);
                // Arrow
                drawRect(panX - 12, rowY + 14, 10, 3, lr, lg, lb);
            }
            drawText(panX + 18, rowY + 14, rows[i].lbl, 3, lr, lg, lb);

            if (!rows[i].isAction) {
                // Bar
                int bx = panX + 200, by = rowY + 16, bw = 200, bh = 10;
                drawRect(bx - 2, by - 2, bw + 4, bh + 4, 0.06f, 0.08f, 0.12f);
                float norm = (rows[i].val - rows[i].mn) / (rows[i].mx - rows[i].mn);
                int fillW2 = (int)(bw * norm);
                if (fillW2 > 0) drawRect(bx, by, fillW2, bh, lr * 1.2f, lg * 1.2f, lb * 0.8f);
                drawRect(bx - 2, by - 2, bw + 4, 2, lr * 0.6f, lg * 0.6f, lb * 0.6f);
                drawRect(bx - 2, by + bh, bw + 4, 2, lr * 0.6f, lg * 0.6f, lb * 0.6f);
                // Value text
                std::ostringstream oss;
                oss << std::fixed;
                oss.precision(rows[i].mx <= 1.0f ? 0 : 1);
                if (rows[i].mx <= 1.0f) oss << (int)(rows[i].val * 100.0f) << " PCT";
                else                    oss << rows[i].val << "X";
                drawText(bx + bw + 8, by, oss.str(), 2, lr, lg, lb);
                if (sel) {
                    drawText(bx - 16, by, "<", 3, lr, lg, lb);
                    drawText(bx + bw + 2, by + 12, ">", 3, lr, lg, lb);
                }
            }
        }

        // Controls panel (shown when row 3 selected)
        // Positioned to the right of the settings panel when there is room;
        // otherwise centred on screen as a modal overlay so it never clips.
        if (settingsSelection == 3)
        {
            int cpW = 320, cpH = 320;
            int cpX, cpY;
            if (panX + panW + 10 + cpW <= width - 8) {
                // Enough room to the right of settings panel
                cpX = panX + panW + 10;
                cpY = panY + 60;
            } else {
                // Fall back: centre on screen as overlay
                cpX = (width  - cpW) / 2;
                cpY = (height - cpH) / 2;
            }
            // Hard-clamp so the popup is always fully on screen
            cpX = std::max(6, std::min(cpX, width  - cpW - 6));
            cpY = std::max(6, std::min(cpY, height - cpH - 6));

            drawRect (cpX, cpY, cpW, cpH, 0.02f, 0.04f, 0.10f);
            drawFrame(cpX, cpY, cpW, cpH, 0.30f, 0.55f, 0.85f);
            // Inner second border for visual depth
            drawFrame(cpX + 4, cpY + 4, cpW - 8, cpH - 8, 0.12f, 0.25f, 0.45f);
            drawText(cpX + 10, cpY + cpH - 24, "CONTROLS", 3, 0.30f, 0.65f, 1.00f);
            drawRect(cpX + 8, cpY + cpH - 30, cpW - 16, 2, 0.20f, 0.40f, 0.65f);

            struct { const char* k; const char* d; } ctrl[] = {
                { "WASD",    "Move Ship"       },
                { "MOUSE",   "Look Around"     },
                { "SHIFT",   "Boost"           },
                { "X",       "EVA / Cockpit"   },
                { "LMB",     "Collect Cell"    },
                { "A / D",   "Tether Minigame" },
                { "C",       "Collect All"     },
                { "H B F",   "HDR Bloom Film"  },
                { "N",       "Night Vision"    },
                { "Q / E",   "Exposure"        },
                { "G",       "Main Menu"       },
                { "ESC",     "Quit"            },
            };
            int nc = (int)(sizeof(ctrl) / sizeof(ctrl[0]));
            for (int ci = 0; ci < nc; ++ci) {
                int cy2 = cpY + cpH - 48 - ci * 22;
                if (cy2 < cpY + 6) break;   // don't draw below panel bottom
                drawText(cpX + 10,  cy2, ctrl[ci].k, 2, 0.25f, 0.75f, 1.00f);
                drawText(cpX + 100, cy2, ctrl[ci].d, 2, 0.60f, 0.65f, 0.65f);
            }
        }

        drawText(panX + 20, panY + 12,
            "W/S SELECT  A/D ADJUST  ESC BACK", 2, 0.40f * nh, 0.65f * nh, 0.40f * nh);
    }
}

// ─── loadHighScores / saveHighScores ─────────────────────────────────────────
void SceneBasic_Uniform::loadHighScores()
{
    highScores.clear();
    FILE* f = fopen("scores.txt", "r");
    if (!f) return;
    int sc; float tm;
    while (fscanf(f, "%d %f", &sc, &tm) == 2)
        highScores.push_back({ sc, tm });
    fclose(f);
    std::sort(highScores.begin(), highScores.end(),
        [](const std::pair<int,float>& a, const std::pair<int,float>& b){ return a.first > b.first; });
    if (highScores.size() > 10) highScores.resize(10);
}

void SceneBasic_Uniform::saveHighScores()
{
    FILE* f = fopen("scores.txt", "w");
    if (!f) return;
    for (auto& hs : highScores) fprintf(f, "%d %.2f\n", hs.first, hs.second);
    fclose(f);
}
#pragma warning(pop)
