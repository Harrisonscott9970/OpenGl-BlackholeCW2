#include "scenebasic_uniform.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

void SceneBasic_Uniform::renderBeaconTowers(const glm::mat4& view, const glm::mat4& proj, float t)
{
    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uIsCore", 0);
    // Beacon towers: polished structural antenna — high metallic, low roughness
    // gives sharp specular highlights that make them read as metal structures
    platformProg.setUniform("uMetallic",  0.45f);
    platformProg.setUniform("uRoughness", 0.28f);
    platformProg.setUniform("uLightPos2", glm::vec3(0.0f));
    platformProg.setUniform("uLightColor2", glm::vec3(0.0f));
    platformProg.setUniform("uLightStrength2", 0.0f);

    glBindVertexArray(platVAO);

    for (const auto& beacon : beaconTowers)
    {
        float pulse = 0.55f + 0.45f * sinf(t * beacon.pulseSpeed + beacon.pulseOffset);
        float flicker = beacon.damaged ? (0.75f + 0.25f * sinf(t * 17.0f + beacon.pulseOffset * 2.0f)) : 1.0f;

        glm::vec3 baseColor = glm::vec3(0.05f, 0.06f, 0.09f);
        glm::vec3 glowColor = beacon.glowColor;
        float glowStrength = (0.7f + pulse * 0.9f) * flicker;

        glm::mat4 model(1.0f);
        model = glm::translate(model, beacon.position + glm::vec3(0.0f, beacon.scale.y * 0.5f, 0.0f));
        model = glm::scale(model, beacon.scale);

        platformProg.setUniform("Model", model);
        platformProg.setUniform("uBaseColor", baseColor);
        platformProg.setUniform("uGlowColor", glowColor);
        platformProg.setUniform("uGlowStrength", glowStrength);

        glDrawArrays(GL_TRIANGLES, 0, platVertCount);
    }

    glBindVertexArray(0);
}

void SceneBasic_Uniform::renderDebrisProps(const glm::mat4& view, const glm::mat4& proj, float t)
{
    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uIsCore", 0);
    // Debris: oxidised/worn scrap metal — low metallic (surface corrosion removes
    // the metallic Fresnel), high roughness (micro-pits from collision damage)
    platformProg.setUniform("uMetallic",  0.18f);
    platformProg.setUniform("uRoughness", 0.78f);
    platformProg.setUniform("uLightPos2", bhPos);
    platformProg.setUniform("uLightColor2", glm::vec3(1.2f, 0.45f, 0.10f));
    platformProg.setUniform("uLightStrength2", 0.18f);

    glBindVertexArray(platVAO);

    for (const auto& debris : debrisProps)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, debris.position);
        model = glm::rotate(model, glm::radians(debris.angle + debris.rotationSpeed * t), debris.rotationAxis);
        model = glm::scale(model, debris.scale);

        platformProg.setUniform("Model", model);
        platformProg.setUniform("uBaseColor", debris.baseColor);
        platformProg.setUniform("uGlowColor", debris.glowColor);
        platformProg.setUniform("uGlowStrength", debris.hazardLit ? 0.35f : 0.18f);

        glDrawArrays(GL_TRIANGLES, 0, platVertCount);
    }

    glBindVertexArray(0);
}

// ─── renderShipBeacon ────────────────────────────────────────────────────────
// NMS Explorer-class ship using a proper triangulated hull mesh (shipVAO).
// The hull faces are actual geometry with correct per-face normals so lighting
// gives hard shading breaks at every edge — reads as a real spacecraft.
// Engine flame orbs (sphereVAO) are drawn additively on top for the glow.
void SceneBasic_Uniform::renderShipBeacon(const glm::mat4& view,
    const glm::mat4& proj, float t)
{
    if (inCockpit) return;

    // ── Colour palette ────────────────────────────────────────────────────────
    const glm::vec3 hullDark(0.07f, 0.08f, 0.10f);   // dark gunmetal
    const glm::vec3 hullLight(0.72f, 0.68f, 0.60f);   // off-white ceramic
    const glm::vec3 accentCyan(0.12f, 0.78f, 1.20f);   // NMS hull stripe
    const glm::vec3 accentOrng(1.05f, 0.48f, 0.08f);   // engine/warning orange
    const glm::vec3 thrustCol(0.25f, 0.80f, 2.00f);   // thrust blue-white

    float pulseA = 0.80f + 0.20f * sinf(t * 9.2f);
    float pulseB = 0.80f + 0.20f * sinf(t * 9.2f + 1.0f);
    float boostMul = boostActive ? 2.5f : 1.0f;

    // ── Ship orientation matrix ───────────────────────────────────────────────
    // Ship orientation is fixed in world space — it points away from the black
    // hole on the XZ plane so it always faces outward like a docked vessel.
    // This decouples the ship from mouse look so it never spins with the camera.
    // Scale: 2.5x so the hull reads clearly at EVA distances.
    glm::vec3 toBHFlat = glm::normalize(glm::vec3(
        bhPos.x - shipPosition.x, 0.f, bhPos.z - shipPosition.z));
    glm::vec3 shipFwd = -toBHFlat;                      // nose points away from BH
    glm::vec3 shipUp = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 shipRt = glm::normalize(glm::cross(shipFwd, shipUp));
    shipUp = glm::normalize(glm::cross(shipRt, shipFwd));

    const float SHIP_SCALE = 2.5f;
    glm::mat4 orient(1.f);
    orient[0] = glm::vec4(shipRt * SHIP_SCALE, 0);
    orient[1] = glm::vec4(shipUp * SHIP_SCALE, 0);
    orient[2] = glm::vec4(shipFwd * SHIP_SCALE, 0);
    orient[3] = glm::vec4(shipPosition, 1.f);

    // ── Draw hull (single draw call) ──────────────────────────────────────────
    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uIsCore", 0);
    platformProg.setUniform("Model", orient);
    platformProg.setUniform("uBaseColor", hullDark);
    platformProg.setUniform("uGlowColor", accentCyan);
    platformProg.setUniform("uGlowStrength", 0.35f + nearDiskSlipstream * 0.8f);
    platformProg.setUniform("uLightPos2", shipPosition + glm::vec3(0, -0.4f, -2.1f));
    platformProg.setUniform("uLightColor2", thrustCol);
    platformProg.setUniform("uLightStrength2", 0.90f);
    platformProg.setUniform("uAlpha", 1.0f);

    // Ship hull: polymer/ceramic composite aerospace material.
    // Low metallic (non-metal composite with metallic paint), moderate roughness.
    platformProg.setUniform("uMetallic",  0.08f);
    platformProg.setUniform("uRoughness", 0.44f);

    glDisable(GL_CULL_FACE);   // hull has open edges — draw both sides
    glBindVertexArray(shipVAO);
    glDrawArrays(GL_TRIANGLES, 0, shipVertCount);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);

    // ── Engine pod flames (additive spheres on nozzle positions) ─────────────
    // Positions in ship local space → world space via orient matrix
    auto worldPt = [&](glm::vec3 local) -> glm::vec3 {
        return glm::vec3(orient * glm::vec4(local, 1.0f));
        };

    auto drawFlame = [&](glm::vec3 localPos, float radius, float strength, float alpha)
        {
            glm::mat4 flameModel = glm::translate(glm::mat4(1.f), worldPt(localPos))
                * glm::scale(glm::mat4(1.f), glm::vec3(radius));
            platformProg.setUniform("Model", flameModel);
            platformProg.setUniform("uIsCore", 1);
            platformProg.setUniform("uBaseColor", glm::vec3(0.01f, 0.03f, 0.06f));
            platformProg.setUniform("uGlowColor", thrustCol);
            platformProg.setUniform("uGlowStrength", strength);
            platformProg.setUniform("uLightPos2", worldPt({ 0,-0.30f,-2.10f }));
            platformProg.setUniform("uLightColor2", thrustCol);
            platformProg.setUniform("uLightStrength2", 0.0f);
            platformProg.setUniform("uAlpha", alpha);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    // Nozzle glow rings (at pod ends — scaled to new PZ1 = -1.35)
    drawFlame({ 0.60f, -0.40f, -1.36f }, 0.16f, 3.8f * pulseA * boostMul, 0.90f);
    drawFlame({ -0.60f, -0.40f, -1.36f }, 0.16f, 3.8f * pulseB * boostMul, 0.90f);
    // Outer glow halos
    drawFlame({ 0.60f, -0.40f, -1.44f }, 0.28f, 1.8f * pulseA * boostMul, 0.55f);
    drawFlame({ -0.60f, -0.40f, -1.44f }, 0.28f, 1.8f * pulseB * boostMul, 0.55f);

    // Cockpit glow (glass fairing — scaled to new nose Z)
    drawFlame({ 0.00f, 0.14f,  1.52f }, 0.20f, 1.2f, 0.35f);

    // Slipstream belly glow (gets brighter near accretion disk)
    if (nearDiskSlipstream > 0.05f) {
        drawFlame({ 0.00f, -0.30f, 0.20f }, 0.30f,
            nearDiskSlipstream * 2.5f, nearDiskSlipstream * 0.6f);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    platformProg.setUniform("uAlpha", 1.0f);
}

// ─── renderCockpit ────────────────────────────────────────────────────────────
// NMS Explorer cockpit: deep instrument panel, three-panel canopy arch,
// side window struts, holographic status displays, animated readout orbs,
// throttle/boost bar, danger-reactive accent colour.
void SceneBasic_Uniform::renderCockpit(const glm::mat4& view,
    const glm::mat4& proj, float t)
{
    if (!inCockpit) return;

    glm::mat4 invView = glm::inverse(view);

    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);

    const glm::vec3 hullDark(0.04f, 0.05f, 0.07f);
    const glm::vec3 hullMid(0.12f, 0.14f, 0.18f);
    const glm::vec3 holoCyan(0.15f, 0.90f, 1.55f);
    const glm::vec3 holoBlue(0.08f, 0.50f, 1.25f);
    const glm::vec3 holoOrng(1.10f, 0.42f, 0.06f);
    const glm::vec3 holoGreen(0.15f, 1.00f, 0.42f);
    const glm::vec3 holoRed(1.20f, 0.12f, 0.06f);
    const glm::vec3 panelGlass(0.02f, 0.06f, 0.10f);

    float pulse = 0.88f + 0.12f * sinf(cockpitPulse * 2.8f);
    glm::vec3 dangerAccent = glm::mix(holoCyan, holoRed, dangerLevel * dangerLevel);

    auto drawHex = [&](glm::vec3 pos, glm::vec3 rotDeg, glm::vec3 sc,
        glm::vec3 base, glm::vec3 glow, float gs, float alpha = 1.f)
        {
            glm::mat4 local = glm::translate(glm::mat4(1.f), pos);
            if (rotDeg.x) local = glm::rotate(local, glm::radians(rotDeg.x), glm::vec3(1, 0, 0));
            if (rotDeg.y) local = glm::rotate(local, glm::radians(rotDeg.y), glm::vec3(0, 1, 0));
            if (rotDeg.z) local = glm::rotate(local, glm::radians(rotDeg.z), glm::vec3(0, 0, 1));
            local = glm::scale(local, sc);
            platformProg.setUniform("Model", invView * local);
            platformProg.setUniform("uIsCore", 0);
            platformProg.setUniform("uBaseColor", base);
            platformProg.setUniform("uGlowColor", glow);
            platformProg.setUniform("uGlowStrength", gs);
            platformProg.setUniform("uLightPos2", glm::vec3(0.f));
            platformProg.setUniform("uLightColor2", glm::vec3(0.f));
            platformProg.setUniform("uLightStrength2", 0.f);
            platformProg.setUniform("uAlpha", alpha);
            glBindVertexArray(platVAO);
            glDrawArrays(GL_TRIANGLES, 0, platVertCount);
            glBindVertexArray(0);
        };

    auto drawSph = [&](glm::vec3 pos, glm::vec3 sc,
        glm::vec3 base, glm::vec3 glow, float gs, float alpha = 1.f)
        {
            glm::mat4 local = glm::translate(glm::mat4(1.f), pos);
            local = glm::scale(local, sc);
            platformProg.setUniform("Model", invView * local);
            platformProg.setUniform("uIsCore", 1);
            platformProg.setUniform("uBaseColor", base);
            platformProg.setUniform("uGlowColor", glow);
            platformProg.setUniform("uGlowStrength", gs);
            platformProg.setUniform("uLightPos2", glm::vec3(0.f));
            platformProg.setUniform("uLightColor2", glm::vec3(0.f));
            platformProg.setUniform("uLightStrength2", 0.f);
            platformProg.setUniform("uAlpha", alpha);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        };

    // ── Solid geometry ────────────────────────────────────────────────────────
    platformProg.setUniform("uIsCore", 0);

    // Instrument panel — wide deep slab
    drawHex({ 0.00f,-2.80f,-2.40f }, { 88.f,0,0 }, { 1.40f,0.09f,0.28f }, hullDark, dangerAccent, 0.08f);
    drawHex({ 0.00f,-2.40f,-2.10f }, { 72.f,0,0 }, { 1.30f,0.06f,0.20f }, hullMid, holoCyan, 0.12f);
    drawHex({ -2.10f,-2.60f,-2.20f }, { 85.f,0,14.f }, { 0.55f,0.07f,0.22f }, hullDark, holoOrng, 0.10f);
    drawHex({ 2.10f,-2.60f,-2.20f }, { 85.f,0,-14.f }, { 0.55f,0.07f,0.22f }, hullDark, holoOrng, 0.10f);

    // Window struts
    drawHex({ -2.55f, 0.20f,-2.30f }, { 0.f,0.f, 26.f }, { 0.06f,0.04f,0.90f }, hullMid, holoCyan, 0.18f);
    drawHex({ 2.55f, 0.20f,-2.30f }, { 0.f,0.f,-26.f }, { 0.06f,0.04f,0.90f }, hullMid, holoCyan, 0.18f);

    // Overhead canopy arch
    drawHex({ 0.00f, 2.00f,-2.50f }, { 0.f,0,0 }, { 1.05f,0.05f,0.28f }, hullMid, holoCyan, 0.14f);

    // ── Holographic displays (additive) ───────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    platformProg.setUniform("uIsCore", 1);

    // Centre main display
    drawHex({ 0.00f,-1.92f,-2.10f }, { 80.f,0,0 }, { 0.60f,0.018f,0.20f },
        panelGlass, holoCyan, 2.6f * pulse, 0.20f);
    drawSph({ 0.00f,-1.62f,-2.00f }, { 0.10f,0.08f,0.10f },
        panelGlass, dangerAccent, 3.8f * pulse, 0.28f);

    // Left nav display (green)
    drawHex({ -1.25f,-2.00f,-2.15f }, { 78.f,0,16.f }, { 0.28f,0.014f,0.16f },
        panelGlass, holoGreen, 2.0f, 0.18f);
    drawSph({ -1.22f,-1.68f,-2.05f }, { 0.07f,0.07f,0.07f },
        panelGlass, holoGreen, 3.0f, 0.22f);

    // Right shield/danger display
    glm::vec3 shieldGlow = glm::mix(holoBlue, holoOrng, dangerLevel);
    drawHex({ 1.25f,-2.00f,-2.15f }, { 78.f,0,-16.f }, { 0.28f,0.014f,0.16f },
        panelGlass, shieldGlow, 2.0f, 0.18f);
    drawSph({ 1.22f,-1.68f,-2.05f }, { 0.07f,0.07f,0.07f },
        panelGlass, shieldGlow, 3.0f, 0.22f);

    // Overhead mission display
    drawHex({ 0.00f, 1.12f,-2.70f }, { 10.f,0,0 }, { 0.35f,0.010f,0.10f },
        panelGlass, holoCyan, 1.8f * pulse, 0.14f);

    // Boost bar (left of centre)
    drawHex({ -0.58f,-2.52f,-1.95f }, { 86.f,0,0 }, { 0.04f,0.008f,0.12f },
        panelGlass, glm::mix(holoCyan, holoOrng, 1.0f - boostEnergy),
        1.6f * boostEnergy, 0.22f);

    // ── Holographic minimap ────────────────────────────────────────────────────
    // A tilted hex-slab acts as the map base; platforms and the BH are shown
    // as glowing orbs at scaled positions around it.
    // Map is positioned on the right side of the instrument panel.

    // Map backing plate (tilted hex slab on right console)
    drawHex({ 1.80f,-2.28f,-2.05f }, { 75.f,0,-8.f }, { 0.32f,0.018f,0.28f },
        glm::vec3(0.01f, 0.02f, 0.04f), holoCyan, 0.55f * pulse, 0.30f);

    // Scale factor: map world units → map plate units
    // Real world radius ~350 units; map plate is ~0.28 local units wide
    const float mapScale = 0.25f / 350.0f;

    // Map centre in camera-local space (centre of the plate)
    const glm::vec3 mapCentre(1.80f, -2.28f, -2.05f);
    const glm::vec3 mapNormal(sinf(glm::radians(-8.f)), cosf(glm::radians(75.f)), sinf(glm::radians(75.f)));

    // Black hole dot (centre of map)
    drawSph(mapCentre + glm::vec3(0, 0.02f, 0), { 0.025f,0.025f,0.025f },
        glm::vec3(0, 0, 0), glm::vec3(2.5f, 0.8f, 0.1f), 3.5f, 0.60f);

    // Platform dots
    for (const auto& zone : platformZones)
    {
        glm::vec3 flatOff = zone.position;   // world position relative to BH (bhPos ~ 0)
        // Project onto XZ plane, scale
        float mx = flatOff.x * mapScale;
        float mz = flatOff.z * mapScale;
        glm::vec3 dotPos = mapCentre + glm::vec3(mx, 0.025f, mz);

        glm::vec3 dotCol = zone.accentColor * 0.8f;
        drawSph(dotPos, { 0.018f,0.018f,0.018f },
            glm::vec3(0.01f, 0.01f, 0.02f), dotCol, 2.8f, 0.55f);
    }

    // Player dot (white)
    {
        glm::vec3 pFlat = camera.Position;
        float px2 = pFlat.x * mapScale;
        float pz2 = pFlat.z * mapScale;
        glm::vec3 pDot = mapCentre + glm::vec3(px2, 0.032f, pz2);
        // Pulse to make player easy to find
        drawSph(pDot, { 0.022f, 0.022f, 0.022f },
            glm::vec3(0.6f, 0.9f, 1.0f), glm::vec3(1.2f, 1.8f, 2.0f), 3.5f * pulse, 0.75f);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    platformProg.setUniform("uAlpha", 1.0f);

    renderCockpitGlass(view, proj, t);
}

// ─── renderCockpitGlass ───────────────────────────────────────────────────────
// Three canopy panels: large centre windscreen + two angled side windows + skylight.
void SceneBasic_Uniform::renderCockpitGlass(const glm::mat4& view,
    const glm::mat4& proj, float t)
{
    if (!inCockpit) return;

    glm::mat4 invView = glm::inverse(view);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uIsCore", 1);
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uLightPos2", glm::vec3(0.f));
    platformProg.setUniform("uLightColor2", glm::vec3(0.f));
    platformProg.setUniform("uLightStrength2", 0.f);

    auto drawPanel = [&](glm::vec3 pos, glm::vec3 rotDeg, glm::vec3 sc, float alpha)
        {
            glm::mat4 local = glm::translate(glm::mat4(1.f), pos);
            if (rotDeg.x) local = glm::rotate(local, glm::radians(rotDeg.x), glm::vec3(1, 0, 0));
            if (rotDeg.y) local = glm::rotate(local, glm::radians(rotDeg.y), glm::vec3(0, 1, 0));
            if (rotDeg.z) local = glm::rotate(local, glm::radians(rotDeg.z), glm::vec3(0, 0, 1));
            local = glm::scale(local, sc);
            platformProg.setUniform("Model", invView * local);
            platformProg.setUniform("uBaseColor", glm::vec3(0.00f, 0.01f, 0.02f));
            platformProg.setUniform("uGlowColor", glm::vec3(0.04f, 0.16f, 0.28f));
            platformProg.setUniform("uGlowStrength", 0.015f);
            platformProg.setUniform("uAlpha", alpha);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        };

    // Centre windscreen
    drawPanel({ 0.00f, 0.55f,-3.50f }, { 0,0,0 }, { 5.20f,1.55f,0.08f }, 0.018f);
    // Left side window
    drawPanel({ -3.10f, 0.20f,-2.80f }, { 0,28.f,0 }, { 2.20f,1.20f,0.08f }, 0.015f);
    // Right side window
    drawPanel({ 3.10f, 0.20f,-2.80f }, { 0,-28.f,0 }, { 2.20f,1.20f,0.08f }, 0.015f);
    // Overhead skylight
    drawPanel({ 0.00f, 2.10f,-2.60f }, { 0,0,0 }, { 3.00f,0.70f,0.08f }, 0.012f);

    platformProg.setUniform("uAlpha", 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ─── drawTetherMinigameUI ────────────────────────────────────────────────────
// Oscilloscope-style "Tether Stabilisation" panel matching the reference image.
// Layout:
//   ┌──────────────────────────────────────────────────────────┐
//   │  TETHER STABILISATION          [A]~~~waveform~~~[D]      │
//   │  TENSION ████████░░░░░░░░░░ 73%                          │
//   │  ┌───────────────[stability window]──────────────────┐   │
//   │  │~~~~~~~~~~~~~●~~~~~~~~~~~~~  ← animated wave+marker│   │
//   │  └──────────────────────────────────────────────────┘   │
//   │  ALIGN WITH THE STABILITY WINDOW                         │
//   │  ●●●○○○○○  lock pips                                     │
//   │  REELING IN…  4.2 m REMAINING                            │
