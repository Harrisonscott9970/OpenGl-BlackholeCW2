#include "scenebasic_uniform.h"
#include "SceneHelpers.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>

// ─── Particle System (Week 7 — Instanced Particles + Week 9 — Noise) ─────────
// Dual-type billboard particle system drawn with one glDrawArraysInstanced call.
//
// Type 0 — Accretion particles (1800):
//   Spiral toward the black hole under a Schwarzschild-inspired gravity term
//   that strengthens as the particle ages, plus 4-octave FBM turbulence for
//   organic swirling drift.  Colour shifts cold-blue → orange → white-hot.
//
// Type 1 — Space dust (700):
//   Large-radius ambient motes that drift slowly with low-frequency FBM noise,
//   never infalling.  Very faint and blue-grey; overlapping additively they
//   build a volumetric haze through the playfield.
//
// GPU-side: each instance carries 7 floats: spawnPos(xyz), age, size, seed, type.
// Billboarding done in the vertex shader via camera-right/up from the view matrix.
//
// References:
//   GPU Gems 3, Ch.23 — instanced particle pattern (instanced VBO layout)
//   Perlin, K. (1985). "An image synthesizer." SIGGRAPH — noise basis in ptFbm()

// ── Type 0: Accretion particle ────────────────────────────────────────────────
void SceneBasic_Uniform::spawnParticle(Particle& p)
{
    float azimuth   = randf(0.0f, glm::two_pi<float>());
    float elevation = randf(-0.25f, 0.25f);
    float radius    = randf(550.0f, 1050.0f);

    p.spawnPos = bhPos + glm::vec3(
        cosf(azimuth) * cosf(elevation) * radius,
        sinf(elevation) * radius * 0.18f,
        sinf(azimuth)  * cosf(elevation) * radius
    );

    p.age      = randf(0.0f, 1.0f);
    p.size     = randf(2.5f, 10.0f);
    p.seed     = randf(0.0f, 100.0f);
    p.type     = 0.0f;
    p.lifespan = randf(6.0f, 18.0f);
}

// ── Type 1: Space dust ────────────────────────────────────────────────────────
// All 400 particles live on four distance rings (260/420/580/740 from origin).
// The home platform at (0, 3.5, 145) sits well inside the 260-unit inner ring,
// keeping spawn a clean dust-free safe zone.
// Ring gap ~160 units → clear empty space between each band.
// Cluster radius 22 units + shader drift 90 → visual cloud radius ~112 units.
void SceneBasic_Uniform::spawnDustParticle(Particle& p)
{
    // All particles go to one of the four outer rings — no home-platform zone.
    const float rings[4] = { 260.0f, 420.0f, 580.0f, 740.0f };
    int   ri   = (int)(randf(0.0f, 4.0f));
    ri = glm::clamp(ri, 0, 3);
    float dist = rings[ri] + randf(-22.0f, 22.0f);

    float az   = randf(0.0f, glm::two_pi<float>());
    float el   = randf(-0.50f, 0.50f);
    glm::vec3 anchor(
        cosf(az) * cosf(el) * dist,
        sinf(el) * dist * 0.48f,
        sinf(az) * cosf(el) * dist
    );

    // Spawn within a tight sphere around the anchor
    float az2 = randf(0.0f, glm::two_pi<float>());
    float el2 = randf(-1.0f,  1.0f);
    float r2  = randf(0.0f, 22.0f);
    float s2  = sqrtf(glm::max(0.0f, 1.0f - el2 * el2));

    p.spawnPos = anchor + glm::vec3(
        cosf(az2) * s2 * r2,
        el2 * r2 * 0.55f,
        sinf(az2) * s2 * r2
    );

    p.age      = randf(0.0f, 1.0f);
    p.size     = randf(5.0f, 15.0f);
    p.seed     = randf(0.0f, 100.0f);
    p.type     = 1.0f;
    p.lifespan = randf(30.0f, 65.0f);
}

void SceneBasic_Uniform::initParticles()
{
    // ── CPU particle data ─────────────────────────────────────────────────────
    particles.resize(MAX_PARTICLES);

    for (int i = 0; i < ACCRETION_COUNT; ++i)
        spawnParticle(particles[i]);
    for (int i = ACCRETION_COUNT; i < MAX_PARTICLES; ++i)
        spawnDustParticle(particles[i]);

    // ── Unit quad geometry (two triangles, -0.5..0.5) ────────────────────────
    float quadVerts[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    glGenVertexArrays(1, &particleQuadVAO);
    glGenBuffers(1, &particleQuadVBO);
    glGenBuffers(1, &particleInstVBO);

    glBindVertexArray(particleQuadVAO);

    // Slot 0: quad vertex positions (vec2, non-instanced)
    glBindBuffer(GL_ARRAY_BUFFER, particleQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);

    // Slots 1-5: per-instance data streamed each frame
    // Layout: vec3 spawnPos | float age | float size | float seed | float type
    //         = 7 floats per instance
    glBindBuffer(GL_ARRAY_BUFFER, particleInstVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 7 * sizeof(float),
        nullptr, GL_STREAM_DRAW);

    const GLsizei instStride = 7 * sizeof(float);

    // location 1: spawnPos (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, instStride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // location 2: age (float)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, instStride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // location 3: size (float)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, instStride, (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // location 4: seed (float)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, instStride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // location 5: type (float) — 0=accretion, 1=space dust
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, instStride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);

    std::cout << "[Particles] Initialised " << ACCRETION_COUNT
              << " accretion + " << DUST_COUNT
              << " space-dust particles (" << MAX_PARTICLES << " total).\n";
}

void SceneBasic_Uniform::updateParticles(float dt)
{
    // Use index to identify particle type — p.type field unreliable on some builds.
    // Dust particles are ALWAYS at indices ACCRETION_COUNT..MAX_PARTICLES-1.
    for (int i = 0; i < (int)particles.size(); ++i)
    {
        auto& p = particles[i];
        p.age += dt / p.lifespan;
        if (p.age >= 1.0f)
        {
            if (i < ACCRETION_COUNT)
                spawnParticle(p);
            else
                spawnDustParticle(p);
        }
    }
}

void SceneBasic_Uniform::renderParticles(const glm::mat4& view,
    const glm::mat4& proj)
{
    if (particles.empty()) return;

    // ── Compute dust-nearby density for the HDR haze overlay ─────────────────
    // Count how many space-dust particles are within HAZE_RADIUS world units
    // of the camera.  Normalise by HAZE_MAX_COUNT so density is 0..1.
    // Dust particles drift up to 80 units from spawnPos in the vertex shader,
    // so use spawnPos + 80 as the effective worst-case radius for proximity.
    // HAZE_RADIUS=180 comfortably catches particles whose drift puts them near camera.
    // Use index to determine particle type — avoids any p.type field issue.
    // Dust = indices ACCRETION_COUNT..MAX_PARTICLES-1
    static constexpr float HAZE_RADIUS    = 90.0f;   // matches new drift amplitude
    static constexpr int   HAZE_MAX_COUNT = 8;        // fewer needed for full haze
    int dustNear = 0;
    for (int i = ACCRETION_COUNT; i < (int)particles.size(); ++i) {
        float d = glm::length(particles[i].spawnPos - camera.Position);
        if (d < HAZE_RADIUS) ++dustNear;
    }
    float rawDensity = glm::clamp((float)dustNear / (float)HAZE_MAX_COUNT, 0.0f, 1.0f);
    dustNearbyDensity = glm::mix(dustNearbyDensity, rawDensity, 0.12f);

    // ── Stream per-instance data to GPU (7 floats per instance) ──────────────
    // Type is explicitly set by index so the shader always sees the correct value
    // regardless of whether p.type was stored correctly in the CPU struct.
    std::vector<float> instData;
    instData.reserve(particles.size() * 7);
    for (int i = 0; i < (int)particles.size(); ++i)
    {
        const auto& p = particles[i];
        instData.push_back(p.spawnPos.x);
        instData.push_back(p.spawnPos.y);
        instData.push_back(p.spawnPos.z);
        instData.push_back(p.age);
        instData.push_back(p.size);
        instData.push_back(p.seed);
        instData.push_back(i < ACCRETION_COUNT ? 0.0f : 1.0f);  // type by index
    }

    glBindBuffer(GL_ARRAY_BUFFER, particleInstVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        instData.size() * sizeof(float), instData.data());

    // ── Draw state ────────────────────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive — particles accumulate brightness
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    particleProg.use();
    particleProg.setUniform("uView",        view);
    particleProg.setUniform("uProjection",  proj);
    particleProg.setUniform("uBHPos",       bhPos);
    particleProg.setUniform("uTime",        (float)glfwGetTime());
    particleProg.setUniform("uNightVision", nightVisionMode ? 1 : 0);

    glBindVertexArray(particleQuadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)particles.size());
    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    // ── Pre-compute dust cluster positions for drawHUD ────────────────────────
    // Sort all dust particles by distance to camera, then pick up to 8 that are
    // at least 40 world units apart.  Stored in dustClusterPositions so drawHUD
    // reads this list directly — no re-filtering of the particle array needed.
    {
        struct DM { float dist; glm::vec3 pos; };
        std::vector<DM> dms;
        dms.reserve(DUST_COUNT);

        // Iterate only the dust portion (index-based — no p.type check needed)
        int dustFound = 0;
        for (int i = ACCRETION_COUNT; i < (int)particles.size(); ++i)
        {
            ++dustFound;
            float d = glm::length(particles[i].spawnPos - camera.Position);
            dms.push_back({ d, particles[i].spawnPos });
        }

        // One-shot diagnostic — should now show DUST_COUNT (700)
        static bool dustDbgPrinted = false;
        if (!dustDbgPrinted) {
            std::cout << "[Dust] cluster compute: found " << dustFound
                      << " dust particles by index (ACCRETION_COUNT=" << ACCRETION_COUNT
                      << ", total=" << particles.size() << ")\n";
            dustDbgPrinted = true;
        }

        std::sort(dms.begin(), dms.end(), [](const DM& a, const DM& b){ return a.dist < b.dist; });

        dustClusterPositions.clear();
        // Primary: pick spread clusters (40-unit dedup)
        for (const auto& dm : dms)
        {
            if ((int)dustClusterPositions.size() >= 8) break;
            bool near = false;
            for (const auto& s : dustClusterPositions)
                if (glm::length(dm.pos - s) < 40.0f) { near = true; break; }
            if (!near) dustClusterPositions.push_back(dm.pos);
        }

        // Fallback: if somehow still empty, just take the 8 closest regardless of spacing
        if (dustClusterPositions.empty() && !dms.empty())
        {
            std::cout << "[Dust] WARNING: cluster dedup produced 0 — using raw closest\n";
            int n = std::min((int)dms.size(), 8);
            for (int i = 0; i < n; ++i)
                dustClusterPositions.push_back(dms[i].pos);
        }

        // Second one-shot to confirm result
        static bool dustDbgResult = false;
        if (!dustDbgResult) {
            std::cout << "[Dust] dustClusterPositions size = "
                      << dustClusterPositions.size() << "\n";
            dustDbgResult = true;
        }
    }
}
