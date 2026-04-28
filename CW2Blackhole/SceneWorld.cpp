#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>

// ─── initAsteroids ───────────────────────────────────────────────────────────
void SceneBasic_Uniform::initAsteroids()
{
    asteroids.clear();
    asteroids.resize(asteroidCount);

    for (int i = 0; i < asteroidCount; i++)
        respawnAsteroid(asteroids[i], true);

    std::cout << "[Spawn] " << asteroidCount << " asteroids seeded into the belt.\n";
}

// ─── respawnAsteroid ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::respawnAsteroid(Asteroid& asteroid, bool randomiseAngle)
{
    asteroid.orbitRadius = randf(asteroidBeltInnerRadius, asteroidBeltOuterRadius);

    if (randomiseAngle || asteroid.angle > glm::two_pi<float>())
        asteroid.angle = randf(0.0f, glm::two_pi<float>());

    asteroid.heightOffset = randf(-asteroidBeltHalfHeight, asteroidBeltHalfHeight);

    float radiusT = 1.0f - ((asteroid.orbitRadius - asteroidBeltInnerRadius) /
        (asteroidBeltOuterRadius - asteroidBeltInnerRadius));

    asteroid.angularSpeed = randf(0.08f, 0.18f) + radiusT * randf(0.05f, 0.16f);
    asteroid.inwardSpeed = randf(8.0f, 26.0f);

    float baseScale = randf(1.0f, 6.5f);
    asteroid.scale = glm::vec3(
        baseScale * randf(0.60f, 1.45f),
        baseScale * randf(0.55f, 1.30f),
        baseScale * randf(0.65f, 1.40f));

    asteroid.rotationAxis = glm::normalize(glm::vec3(
        randf(-1.0f, 1.0f),
        randf(-1.0f, 1.0f),
        randf(-1.0f, 1.0f)));

    if (glm::length(asteroid.rotationAxis) < 0.001f)
        asteroid.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

    asteroid.tiltAngle = randf(-32.0f, 32.0f);
    asteroid.rotationAngle = randf(0.0f, 360.0f);
    asteroid.rotationSpeed = randf(8.0f, 70.0f);
    asteroid.wobblePhase = randf(0.0f, glm::two_pi<float>());

    asteroid.position = bhPos + glm::vec3(
        cosf(asteroid.angle) * asteroid.orbitRadius,
        asteroid.heightOffset,
        sinf(asteroid.angle) * asteroid.orbitRadius);

    asteroid.active = true;
}

// ─── updateAsteroids ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::updateAsteroids(float dt)
{
    if (asteroids.empty()) return;

    for (auto& asteroid : asteroids)
    {
        if (!asteroid.active) continue;

        glm::vec3 offset = asteroid.position - bhPos;
        float dist = glm::length(offset);
        if (dist < 0.001f) dist = 0.001f;

        float pullT = 1.0f - glm::clamp((dist - asteroidConsumeRadius) /
            glm::max(0.001f, asteroidPullRadius - asteroidConsumeRadius), 0.0f, 1.0f);
        pullT = glm::clamp(pullT, 0.0f, 1.0f);

        float difficultyMul = 1.0f + difficultyTier * 0.45f;
        asteroid.angle += asteroid.angularSpeed * dt * difficultyMul * (1.0f + pullT * 1.65f);

        float inwardThisFrame = asteroid.inwardSpeed * dt * difficultyMul;
        inwardThisFrame += pullT * pullT * 125.0f * dt;
        asteroid.orbitRadius -= inwardThisFrame;

        float wobble = sinf((float)glfwGetTime() * (0.35f + asteroid.angularSpeed) + asteroid.wobblePhase)
            * (4.0f + asteroid.scale.x * 0.25f);
        float collapse = pullT * 18.0f;
        if (asteroid.heightOffset > 0.0f)
            asteroid.heightOffset = glm::max(0.0f, asteroid.heightOffset - collapse * dt);
        else
            asteroid.heightOffset = glm::min(0.0f, asteroid.heightOffset + collapse * dt);

        asteroid.position = bhPos + glm::vec3(
            cosf(asteroid.angle) * asteroid.orbitRadius,
            asteroid.heightOffset + wobble,
            sinf(asteroid.angle) * asteroid.orbitRadius);

        glm::vec3 inwardDir = glm::normalize(bhPos - asteroid.position);
        asteroid.position += inwardDir * (pullT * (55.0f + 16.0f * difficultyTier) * dt);

        asteroid.rotationAngle += asteroid.rotationSpeed * dt * difficultyMul * (1.0f + pullT);
        if (asteroid.rotationAngle > 360.0f)
            asteroid.rotationAngle = fmodf(asteroid.rotationAngle, 360.0f);

        if (asteroid.orbitRadius <= asteroidConsumeRadius ||
            glm::length(asteroid.position - bhPos) <= asteroidConsumeRadius)
        {
            respawnAsteroid(asteroid, true);
        }
    }
}

// ─── renderAsteroids ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::renderAsteroids(const glm::mat4& view, const glm::mat4& proj, float t)
{
    if (asteroids.empty()) return;
    setPlatformUniforms(view, proj, t);
    platformProg.use();
    platformProg.setUniform("uIsCore", 0);
    platformProg.setUniform("uHasDiffuse", 0);
    platformProg.setUniform("uHasNormal", 0);
    platformProg.setUniform("uMetallic",  0.00f);
    platformProg.setUniform("uRoughness", 0.92f);
    glBindVertexArray(sphereVAO);
    for (const auto& asteroid : asteroids) {
        if (!asteroid.active) continue;
        float dist = glm::length(asteroid.position - bhPos);
        float pullT = 1.0f - glm::clamp((dist - asteroidConsumeRadius) /
            glm::max(0.001f, asteroidPullRadius - asteroidConsumeRadius), 0.0f, 1.0f);
        pullT = glm::clamp(pullT, 0.0f, 1.0f);
        glm::vec3 baseColor = glm::mix(glm::vec3(0.06f,0.06f,0.07f), glm::vec3(0.22f,0.14f,0.08f), pullT*0.45f);
        glm::vec3 glowColor = glm::mix(glm::vec3(0.04f,0.04f,0.05f), glm::vec3(1.80f,0.55f,0.08f), pullT*0.95f);
        float glowStrength = 0.05f + pullT * 1.40f;
        glm::mat4 model(1.0f);
        model = glm::translate(model, asteroid.position);
        model = glm::rotate(model, asteroid.angle + glm::radians(asteroid.tiltAngle), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(asteroid.rotationAngle), asteroid.rotationAxis);
        model = glm::scale(model, asteroid.scale);
        platformProg.setUniform("Model", model);
        platformProg.setUniform("uBaseColor", baseColor);
        platformProg.setUniform("uGlowColor", glowColor);
        platformProg.setUniform("uGlowStrength", glowStrength);
        platformProg.setUniform("uLightPos2", bhPos);
        platformProg.setUniform("uLightColor2", glm::vec3(1.25f, 0.52f, 0.12f));
        platformProg.setUniform("uLightStrength2", 0.24f + pullT * 0.95f);
        glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

// ─── spawnPlatformsAndCells ──────────────────────────────────────────────────
void SceneBasic_Uniform::spawnPlatformsAndCells()
{
    platformZones.clear();
    energyCells.clear();
    beaconTowers.clear();
    debrisProps.clear();

    srand((unsigned int)time(nullptr));

    // CW2: Expanded world — 7-9 platforms spread across a much wider area
    // with 3 orbital rings (close, mid, far) to give the space depth and
    // encourage players to venture toward the black hole for slipstream bonuses.
    int count = 7 + (rand() % 3);   // was 4-5, now 7-9
    const float PI2 = 6.28318530f;

    // Ring radii — close ring is just outside safe zone, far ring brushes
    // the inner edge of the asteroid belt for a genuinely dangerous run
    const float ringRadii[3] = { 85.0f, 180.0f, 310.0f };
    const float ringYSpread[3] = { 10.0f, 22.0f,  38.0f };
    const float platformY = 0.8f;

    for (int i = 0; i < count; i++)
    {
        // Distribute across rings: first platform is always home (ring 0),
        // rest spread across all three rings with increasing hazard
        int ring = (i == 0) ? 0 : (1 + (i % 2 == 0 ? 2 : 1) + (rand() % 2)) % 3;

        float baseAngle = (float)i / count * PI2;
        float jitter = ((float)(rand() % 1000) / 1000.f - 0.5f) * (PI2 / count) * 0.5f;
        float angle = baseAngle + jitter;

        float dist = ringRadii[ring] + randf(-25.0f, 25.0f);
        float yOff = randf(-ringYSpread[ring], ringYSpread[ring]);

        PlatformZone zone;
        zone.position = glm::vec3(cosf(angle) * dist, platformY + yOff, sinf(angle) * dist);

        if (i == 0)
            zone.type = PlatformZoneType::HomeRelay;
        else if (ring == 2)
            zone.type = PlatformZoneType::HazardRelay;   // far ring = always hazard
        else if (ring == 1 && (rand() % 100) < 40)
            zone.type = PlatformZoneType::DamagedRelay;
        else if ((rand() % 100) < 25)
            zone.type = PlatformZoneType::DamagedRelay;
        else
            zone.type = PlatformZoneType::StableRelay;

        zone.accentColor = getZoneAccentColor(zone.type);
        zone.glowStrength = getZoneGlowStrength(zone.type);
        zone.localDangerBias = (zone.type == PlatformZoneType::HazardRelay) ? 0.35f :
            (zone.type == PlatformZoneType::DamagedRelay) ? 0.14f : 0.0f;

        platformZones.push_back(zone);

        // Every platform has a cell scattered 8-16 units from the platform centre
        // (not directly above it) so the player has to navigate to pick them up.
        {
            EnergyCell cell;
            cell.platformPos = zone.position;
            float ca  = randf(0.0f, glm::two_pi<float>());
            float cr  = randf(8.0f, 16.0f);
            cell.position = zone.position + glm::vec3(
                cosf(ca) * cr,
                randf(2.0f, 5.5f),
                sinf(ca) * cr
            );
            energyCells.push_back(cell);
        }

        if (ring == 2)   // bonus cell on hazard platforms, further out
        {
            EnergyCell cell2;
            cell2.platformPos = zone.position;
            float ca2 = randf(0.0f, glm::two_pi<float>());
            float cr2 = randf(14.0f, 24.0f);
            cell2.position = zone.position + glm::vec3(
                cosf(ca2) * cr2,
                randf(3.0f, 7.0f),
                sinf(ca2) * cr2
            );
            energyCells.push_back(cell2);
        }
    }

    spawnPlatformSetDressings();

    std::cout << "[Spawn] " << platformZones.size() << " relay zones, "
        << energyCells.size() << " energy cells.\n";
}

glm::vec3 SceneBasic_Uniform::getZoneAccentColor(PlatformZoneType type) const
{
    switch (type)
    {
    case PlatformZoneType::HomeRelay:    return glm::vec3(0.15f, 0.95f, 1.60f);  // bright cyan
    case PlatformZoneType::StableRelay:  return glm::vec3(0.10f, 0.70f, 1.40f);  // blue-cyan
    case PlatformZoneType::DamagedRelay: return glm::vec3(1.40f, 0.42f, 0.06f);  // warning orange
    case PlatformZoneType::HazardRelay:  return glm::vec3(1.60f, 0.18f, 0.08f);  // danger red
    default:                             return glm::vec3(0.10f, 0.70f, 1.40f);
    }
}

float SceneBasic_Uniform::getZoneGlowStrength(PlatformZoneType type) const
{
    // Platform DECK glow strength — kept low so the PBR metallic appearance
    // dominates.  The rim formula is  0.08 + glowStrength * 0.16;  at 0.40
    // that is 0.144 — a tight edge catch-light.  Old values (1.8–2.4) gave
    // 0.37–0.46, which overwhelmed the dark albedo and made every platform
    // surface look brightly lit and cyan from any sideways viewing angle.
    // Beacon towers, energy cells, and debris have their own glow uniforms
    // and are unaffected by this table.
    switch (type)
    {
    case PlatformZoneType::HomeRelay:    return 0.40f;
    case PlatformZoneType::StableRelay:  return 0.32f;
    case PlatformZoneType::DamagedRelay: return 0.28f;  // corroded — even dimmer
    case PlatformZoneType::HazardRelay:  return 0.48f;  // slightly brighter warning red
    default:                             return 0.32f;
    }
}

void SceneBasic_Uniform::spawnPlatformSetDressings()
{
    beaconTowers.clear();
    debrisProps.clear();

    for (const auto& zone : platformZones)
    {
        int beaconCount = 1;
        int debrisCount = 6;

        switch (zone.type)
        {
        case PlatformZoneType::HomeRelay:
            beaconCount = 2;
            debrisCount = 5;
            break;
        case PlatformZoneType::StableRelay:
            beaconCount = 1;
            debrisCount = 7;
            break;
        case PlatformZoneType::DamagedRelay:
            beaconCount = 2;
            debrisCount = 10;
            break;
        case PlatformZoneType::HazardRelay:
            beaconCount = 1;
            debrisCount = 13;
            break;
        }

        for (int i = 0; i < beaconCount; ++i)
        {
            float a = randf(0.0f, glm::two_pi<float>());
            float r = randf(8.5f, 14.0f);

            BeaconTower beacon;
            beacon.position = zone.position + glm::vec3(cosf(a) * r, 0.0f, sinf(a) * r);
            beacon.scale = glm::vec3(randf(0.35f, 0.55f), randf(3.0f, 5.8f), randf(0.35f, 0.55f));
            beacon.glowColor = zone.accentColor;
            beacon.pulseSpeed = randf(0.9f, 1.8f);
            beacon.pulseOffset = randf(0.0f, 20.0f);
            beacon.damaged = (zone.type == PlatformZoneType::DamagedRelay);
            beaconTowers.push_back(beacon);
        }

        for (int i = 0; i < debrisCount; ++i)
        {
            float a = randf(0.0f, glm::two_pi<float>());
            // Spread debris in two bands: close (near platform) and far (floating away)
            float r = (i % 3 == 0)
                ? randf(6.0f,  16.0f)    // close cluster
                : randf(18.0f, 38.0f);   // wider scattered field

            DebrisProp debris;
            debris.position = zone.position + glm::vec3(
                cosf(a) * r,
                randf(-3.5f, 6.5f),
                sinf(a) * r
            );

            debris.rotationAxis = glm::normalize(glm::vec3(
                randf(-1.0f, 1.0f),
                randf(-1.0f, 1.0f),
                randf(-1.0f, 1.0f)
            ));

            if (glm::length(debris.rotationAxis) < 0.001f)
                debris.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

            // Shape variety: pick a random archetype each debris piece
            //   0 — rounded boulder    (roughly uniform scale, small-medium)
            //   1 — elongated shard    (one axis much longer, like a splinter)
            //   2 — flat pancake plate (squashed Y, wide XZ)
            //   3 — stubby chunk       (compact but non-uniform)
            int shapeType = rand() % 4;
            switch (shapeType)
            {
            case 0: // Rounded boulder
                {
                    float s = randf(0.30f, 1.10f);
                    debris.scale = glm::vec3(
                        s * randf(0.80f, 1.20f),
                        s * randf(0.75f, 1.15f),
                        s * randf(0.80f, 1.25f));
                }
                break;
            case 1: // Elongated shard / splinter
                {
                    float len = randf(0.90f, 2.20f);
                    float wid = randf(0.12f, 0.38f);
                    debris.scale = glm::vec3(wid, wid * randf(0.8f, 1.2f), len);
                }
                break;
            case 2: // Flat plate / disk fragment
                {
                    float radius = randf(0.50f, 1.40f);
                    float thick  = randf(0.08f, 0.22f);
                    debris.scale = glm::vec3(radius, thick, radius * randf(0.7f, 1.3f));
                }
                break;
            case 3: // Stubby irregular chunk
            default:
                {
                    debris.scale = glm::vec3(
                        randf(0.20f, 0.90f),
                        randf(0.20f, 0.60f),
                        randf(0.25f, 0.85f));
                }
                break;
            }

            debris.baseColor = (zone.type == PlatformZoneType::HazardRelay)
                ? glm::vec3(0.16f, 0.11f, 0.09f)
                : glm::vec3(0.07f, 0.09f, 0.13f);

            // 30% chance of faint accent glow (damaged/hazard pieces glow more)
            float glowChance = (zone.type == PlatformZoneType::DamagedRelay ||
                                zone.type == PlatformZoneType::HazardRelay) ? 40.0f : 22.0f;
            debris.glowColor = ((rand() % 100) < (int)glowChance)
                ? zone.accentColor * 0.40f
                : glm::vec3(0.0f);

            debris.rotationSpeed = randf(4.0f, 32.0f);
            debris.angle         = randf(0.0f, 360.0f);
            debris.hazardLit     = (zone.type == PlatformZoneType::HazardRelay);

            debrisProps.push_back(debris);
        }
    }
}

void SceneBasic_Uniform::renderCell(const EnergyCell& cell,
    const glm::mat4& view, const glm::mat4& proj, float t)
{
    if (cell.cellScale > 0.01f)
    {
        float yRot = (cell.phase == PickupPhase::Idle) ? t * 0.8f : cell.spinAngle;

        glm::mat4 model =
            glm::translate(glm::mat4(1.f), cell.position) *
            glm::rotate(glm::mat4(1.f), yRot, glm::vec3(0, 1, 0)) *
            glm::scale(glm::mat4(1.f), glm::vec3(0.55f * cell.cellScale));

        // Night-vision mode: cells are dimmed to nearly invisible — the player
        // must navigate by dust landmarks instead of bright cell beacons.
        float cellGlow = nightVisionMode ? 0.04f : 3.5f;
        glm::vec3 cellBase  = nightVisionMode
            ? glm::vec3(0.005f, 0.005f, 0.005f)
            : glm::vec3(0.05f,  0.12f,  0.35f);
        glm::vec3 cellColor = nightVisionMode
            ? glm::vec3(0.01f, 0.02f, 0.03f)
            : glm::vec3(0.20f, 0.75f, 2.20f);

        platformProg.use();
        platformProg.setUniform("Model", model);
        platformProg.setUniform("uTime", t);          // energy line animation
        platformProg.setUniform("uBaseColor",    cellBase);
        platformProg.setUniform("uGlowColor",    cellColor);
        platformProg.setUniform("uGlowStrength", cellGlow);
        platformProg.setUniform("uIsCore", 1);
        platformProg.setUniform("uHasDiffuse", 0);
        platformProg.setUniform("uHasNormal",  0);
        platformProg.setUniform("uLightPos2", cell.position);
        platformProg.setUniform("uLightColor2", glm::vec3(0.25f, 0.80f, 1.35f));
        platformProg.setUniform("uLightStrength2", 0.45f);

        // Use Blender-exported cell mesh if loaded; fallback to procedural sphere.
        // Both VAOs share the same 14-float stride so platformProg works with either.
        if (cellMesh.loaded) {
            glBindVertexArray(cellMesh.vao);
            glDrawElements(GL_TRIANGLES, cellMesh.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        } else {
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
    }

    if (cell.phase == PickupPhase::SpinShrink || cell.phase == PickupPhase::Poof)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        platformProg.use();
        platformProg.setUniform("uTime", t);
        platformProg.setUniform("uIsCore", 1);
        platformProg.setUniform("uHasDiffuse", 0);
        platformProg.setUniform("uHasNormal", 0);
        platformProg.setUniform("uLightPos2", cell.position);
        platformProg.setUniform("uLightColor2", glm::vec3(0.25f, 0.80f, 1.50f));
        platformProg.setUniform("uLightStrength2", 0.55f);

        if (cell.phase == PickupPhase::SpinShrink)
        {
            float r = 0.6f + cell.orbScale * 3.0f;
            float a = cell.orbScale * 2.0f;

            glm::mat4 m = glm::translate(glm::mat4(1.f), cell.position) *
                glm::scale(glm::mat4(1.f), glm::vec3(r));
            platformProg.setUniform("Model", m);
            platformProg.setUniform("uBaseColor", glm::vec3(0.0f, 0.1f, 0.4f) * a);
            platformProg.setUniform("uGlowColor", glm::vec3(0.1f, 0.5f, 2.0f) * a);
            platformProg.setUniform("uGlowStrength", 3.0f * a);

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
        else
        {
            float p = 1.0f - cell.poofAlpha;

            struct RingDef { float speed; float bright; float offset; };
            RingDef rings[3] = {
                { 1.0f,  1.0f,  0.0f },
                { 0.65f, 0.7f,  0.3f },
                { 0.35f, 0.4f,  0.6f }
            };

            for (auto& rd : rings)
            {
                float rp = glm::clamp(p * rd.speed, 0.f, 1.f);
                float alpha = cell.poofAlpha * rd.bright;
                float r = 0.5f + rp * 9.0f + rd.offset;

                glm::mat4 m = glm::translate(glm::mat4(1.f), cell.position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(r));
                platformProg.setUniform("Model", m);
                platformProg.setUniform("uBaseColor", glm::vec3(0.0f, 0.15f, 0.5f) * alpha);
                platformProg.setUniform("uGlowColor", glm::vec3(0.1f, 0.55f, 2.2f) * alpha);
                platformProg.setUniform("uGlowStrength", 4.0f * alpha);

                glBindVertexArray(sphereVAO);
                glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

// ─── setPlatformUniforms ─────────────────────────────────────────────────────
void SceneBasic_Uniform::setPlatformUniforms(const glm::mat4& view,
    const glm::mat4& proj, float t)
{
    platformProg.use();
    platformProg.setUniform("View", view);
    platformProg.setUniform("Projection", proj);
    platformProg.setUniform("uLightPos", lightPos);
    platformProg.setUniform("uCamPos", camera.Position);
    platformProg.setUniform("uTime", t);
    platformProg.setUniform("uBHPos", bhPos);
    platformProg.setUniform("uDiffuseTex", 3);
    platformProg.setUniform("uNormalTex", 4);
    platformProg.setUniform("uAlpha", 1.0f);

    platformProg.setUniform("uLightPos2", glm::vec3(0.0f));
    platformProg.setUniform("uLightColor2", glm::vec3(0.0f));
    platformProg.setUniform("uLightStrength2", 0.0f);

    // Week 10 — PBR material defaults.
    // Values here act as a safe fallback; each draw group overrides them to
    // represent its physical material (rock, worn metal, polished antenna, etc.).
    // metallic  0 = dielectric (plastic/rock/ceramic), 1 = pure metal
    // roughness 0 = mirror-smooth, 1 = fully diffuse micro-facet surface
    // Reference: Cook & Torrance (1982), Walter et al. (2007), Schlick (1994)
    //            — see platform.frag for full citation block.
    platformProg.setUniform("uMetallic",  0.12f);   // general space-metal default
    platformProg.setUniform("uRoughness", 0.58f);   // moderately rough metal

    // Week 9 Noise: default to no dissolve — only DamagedRelay / HazardRelay
    // zone platforms override this to a non-zero value (set in render() zone loop).
    platformProg.setUniform("uDissolve",  0.00f);

    // Shadow map uniforms are set once per frame in render() before the first
    // platform draw — calling setShadowUniforms() here would redundantly rebind
    // the depth texture and rewrite the same uniforms for every draw group.
}

