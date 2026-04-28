#include "scenebasic_uniform.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>       // loadOBJMesh
#include <sstream>       // loadOBJMesh
#include <map>           // loadOBJMesh vertex deduplication
#include <tuple>         // loadOBJMesh index key
#include <cmath>         // fabsf

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244 4996 6262 6308 6386 26451 26819)
#include "helper/stb/stb_image.h"
#pragma warning(pop)

// Skybox cube vertices
static float s_skyboxVerts[] = {
    -1, 1,-1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
    -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
     1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
    -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
    -1,-1,-1, -1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1, 1,  1,-1, 1
};

// ─── buildSphereMesh ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::buildSphereMesh(int stacks, int slices)
{
    std::vector<float> verts;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265f;
    const float PI2 = 6.28318530f;

    for (int i = 0; i <= stacks; i++) {
        float phi = PI * (float)i / stacks;
        float sinPhi = sinf(phi), cosPhi = cosf(phi);

        for (int j = 0; j <= slices; j++) {
            float theta = PI2 * (float)j / slices;
            float x = sinPhi * cosf(theta);
            float y = cosPhi;
            float z = sinPhi * sinf(theta);

            float tx = -sinf(theta);
            float ty = 0.0f;
            float tz = cosf(theta);
            float bx = cosPhi * cosf(theta);
            float by = -sinPhi;
            float bz = cosPhi * sinf(theta);

            verts.insert(verts.end(), {
                x, y, z,
                x, y, z,
                (float)j / slices, (float)i / stacks,
                tx, ty, tz,
                bx, by, bz
                });
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int r1 = i * (slices + 1);
            int r2 = (i + 1) * (slices + 1);
            indices.push_back(r1 + j);
            indices.push_back(r2 + j);
            indices.push_back(r2 + j + 1);
            indices.push_back(r1 + j);
            indices.push_back(r2 + j + 1);
            indices.push_back(r1 + j + 1);
        }
    }

    sphereIdxCount = (int)indices.size();
    const int stride = 14;

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

// ─── buildDiskMesh ───────────────────────────────────────────────────────────
void SceneBasic_Uniform::buildDiskMesh()
{
    std::vector<float> verts;
    const int   rings = 120;
    const int   slices = 300;
    const float inner = 1.02f;
    const float outer = 2.2f;
    const float PI2 = 6.28318530f;

    for (int i = 0; i < rings; i++) {
        float r0 = inner + (outer - inner) * (float)i / rings;
        float r1 = inner + (outer - inner) * (float)(i + 1) / rings;

        for (int j = 0; j < slices; j++) {
            float a0 = (float)j / slices * PI2;
            float a1 = (float)(j + 1) / slices * PI2;

            glm::vec3 p00(cosf(a0) * r0, 0, sinf(a0) * r0);
            glm::vec3 p10(cosf(a1) * r0, 0, sinf(a1) * r0);
            glm::vec3 p01(cosf(a0) * r1, 0, sinf(a0) * r1);
            glm::vec3 p11(cosf(a1) * r1, 0, sinf(a1) * r1);

            verts.insert(verts.end(), { p00.x,p00.y,p00.z });
            verts.insert(verts.end(), { p10.x,p10.y,p10.z });
            verts.insert(verts.end(), { p11.x,p11.y,p11.z });
            verts.insert(verts.end(), { p00.x,p00.y,p00.z });
            verts.insert(verts.end(), { p11.x,p11.y,p11.z });
            verts.insert(verts.end(), { p01.x,p01.y,p01.z });
        }
    }

    diskVertCount = (int)verts.size() / 3;

    glGenVertexArrays(1, &diskVAO);
    glGenBuffers(1, &diskVBO);

    glBindVertexArray(diskVAO);
    glBindBuffer(GL_ARRAY_BUFFER, diskVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ─── buildPlatformMesh ───────────────────────────────────────────────────────
// Hexagonal platform: top face + side walls + bottom cap.
// H increased from 0.3 to 1.4 so the platform reads as a solid relay slab.
// Bottom cap (reversed winding) fixes the hollow/vanishing look from below.
void SceneBasic_Uniform::buildPlatformMesh()
{
    std::vector<float> verts;
    const int   sides = 6;
    const float R = 6.0f;
    const float H = 1.4f;   // thicker slab (was 0.3 — too thin, looked hollow)
    const float PI2 = 6.28318530f;

    auto pushVert = [&](glm::vec3 pos, glm::vec3 n, glm::vec2 uv, glm::vec3 tangent)
        {
            glm::vec3 bt = glm::normalize(glm::cross(n, tangent));
            verts.insert(verts.end(), {
                pos.x, pos.y, pos.z,
                n.x,   n.y,   n.z,
                uv.x,  uv.y,
                tangent.x, tangent.y, tangent.z,
                bt.x, bt.y, bt.z
                });
        };

    // ── Top face (Y = H, normal up) ───────────────────────────────────────────
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i / sides * PI2;
        float a1 = (float)(i + 1) / sides * PI2;

        glm::vec3 c(0, H, 0);
        glm::vec3 p0(cosf(a0) * R, H, sinf(a0) * R);
        glm::vec3 p1(cosf(a1) * R, H, sinf(a1) * R);
        glm::vec3 n(0, 1, 0);
        glm::vec3 t(1, 0, 0);

        pushVert(c, n, { 0.5f, 0.5f }, t);
        pushVert(p0, n, { cosf(a0) * .5f + .5f, sinf(a0) * .5f + .5f },
            glm::normalize(glm::vec3(cosf(a0), 0, sinf(a0))));
        pushVert(p1, n, { cosf(a1) * .5f + .5f, sinf(a1) * .5f + .5f },
            glm::normalize(glm::vec3(cosf(a1), 0, sinf(a1))));
    }

    // ── Side walls (outward normals) ──────────────────────────────────────────
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i / sides * PI2;
        float a1 = (float)(i + 1) / sides * PI2;

        glm::vec3 tl(cosf(a0) * R, H, sinf(a0) * R);
        glm::vec3 tr(cosf(a1) * R, H, sinf(a1) * R);
        glm::vec3 bl(cosf(a0) * R, 0, sinf(a0) * R);
        glm::vec3 br(cosf(a1) * R, 0, sinf(a1) * R);

        float am = (a0 + a1) * .5f;
        glm::vec3 n(cosf(am), 0, sinf(am));
        glm::vec3 t = glm::normalize(glm::vec3(-sinf(am), 0, cosf(am)));

        pushVert(tl, n, { 0,1 }, t);
        pushVert(bl, n, { 0,0 }, t);
        pushVert(br, n, { 1,0 }, t);
        pushVert(tl, n, { 0,1 }, t);
        pushVert(br, n, { 1,0 }, t);
        pushVert(tr, n, { 1,1 }, t);
    }

    // ── Bottom cap (Y = 0, normal DOWN) — fixes hollow look from below ────────
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i / sides * PI2;
        float a1 = (float)(i + 1) / sides * PI2;

        glm::vec3 c(0, 0, 0);
        glm::vec3 p0(cosf(a0) * R, 0, sinf(a0) * R);
        glm::vec3 p1(cosf(a1) * R, 0, sinf(a1) * R);
        glm::vec3 n(0, -1, 0);
        glm::vec3 t(1, 0, 0);

        // Reversed winding (centre, p1, p0) so normal faces down
        pushVert(c, n, { 0.5f, 0.5f }, t);
        pushVert(p1, n, { cosf(a1) * .5f + .5f, sinf(a1) * .5f + .5f },
            glm::normalize(glm::vec3(cosf(a1), 0, sinf(a1))));
        pushVert(p0, n, { cosf(a0) * .5f + .5f, sinf(a0) * .5f + .5f },
            glm::normalize(glm::vec3(cosf(a0), 0, sinf(a0))));
    }

    platVertCount = (int)verts.size() / 14;

    glGenVertexArrays(1, &platVAO);
    glGenBuffers(1, &platVBO);

    glBindVertexArray(platVAO);
    glBindBuffer(GL_ARRAY_BUFFER, platVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    const GLsizei stride = 14 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}


// ─── buildShipMesh ────────────────────────────────────────────────────────────
// Generates a proper low-poly NMS Explorer hull entirely from triangles.
// The mesh is built in local ship space (nose = +Z, tail = -Z, up = +Y).
// Uses the same 14-float stride as sphereVAO/platVAO (pos/norm/uv/tan/bitan)
// so it plugs straight into platformProg (Blinn-Phong + PCF shadows).
//
// Hull design:
//   • Central fuselage — tapered octagonal prism, nose cone, tail flare
//   • Two swept delta wings with sharp leading/trailing edges
//   • Twin engine pods under the tail with flat nozzle caps
//   • Raised cockpit fairing with angled glass face
//   • Two dorsal fin stubs
//
// All geometry is hand-authored via anchor points so faces have clean normals
// and silhouette edges read well against the space background.
void SceneBasic_Uniform::buildShipMesh()
{
    // We accumulate triangles as flat float arrays (pos3 norm3 uv2 tan3 bitan3)
    std::vector<float> verts;

    // Helper: push one triangle. Normals/tangents computed from positions.
    auto pushTri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c,
        glm::vec2 uva = { 0,0 }, glm::vec2 uvb = { 1,0 }, glm::vec2 uvc = { 0.5f,1 })
        {
            glm::vec3 edge1 = b - a;
            glm::vec3 edge2 = c - a;
            glm::vec3 n = glm::normalize(glm::cross(edge1, edge2));
            glm::vec3 t = glm::normalize(edge1);
            glm::vec3 bt = glm::normalize(glm::cross(n, t));

            auto push = [&](glm::vec3 p, glm::vec2 uv) {
                verts.insert(verts.end(), {
                    p.x, p.y, p.z,
                    n.x, n.y, n.z,
                    uv.x, uv.y,
                    t.x, t.y, t.z,
                    bt.x, bt.y, bt.z
                    });
                };
            push(a, uva);
            push(b, uvb);
            push(c, uvc);
        };

    // Helper: push a quad (two triangles, consistent winding)
    auto pushQuad = [&](glm::vec3 tl, glm::vec3 tr, glm::vec3 bl, glm::vec3 br)
        {
            pushTri(tl, tr, bl, { 0,1 }, { 1,1 }, { 0,0 });
            pushTri(tr, br, bl, { 1,1 }, { 1,0 }, { 0,0 });
        };

    // Mirror helper: push a tri and its Y-mirrored twin (for symmetric wings/fins)
    auto pushTriMirrorX = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c,
        glm::vec2 uva = { 0,0 }, glm::vec2 uvb = { 1,0 }, glm::vec2 uvc = { 0.5f,1 })
        {
            pushTri(a, b, c, uva, uvb, uvc);
            // Mirror X: flip x, also flip winding to keep normals outward
            pushTri({ -a.x,a.y,a.z }, { -c.x,c.y,c.z }, { -b.x,b.y,b.z }, uva, uvc, uvb);
        };

    auto pushQuadMirrorX = [&](glm::vec3 tl, glm::vec3 tr, glm::vec3 bl, glm::vec3 br)
        {
            pushQuad(tl, tr, bl, br);
            // Mirrored side (flip X, also flip winding)
            pushQuad({ -tr.x,tr.y,tr.z }, { -tl.x,tl.y,tl.z },
                { -br.x,br.y,br.z }, { -bl.x,bl.y,bl.z });
        };

    // ── Fuselage spine anchors — SHORTER & THICKER ───────────────────────────
    // Z compressed to ~60% of original, X/Y expanded to ~140% for a stocky look.
    // Nose tip
    glm::vec3 noseTip(0.00f, 0.03f, 1.90f);
    // Cockpit ring (just behind nose)
    glm::vec3 ckTop(0.00f, 0.52f, 1.40f);
    glm::vec3 ckSide(0.38f, 0.14f, 1.40f);
    glm::vec3 ckBot(0.00f, -0.25f, 1.40f);
    // Main hull forward ring
    glm::vec3 fwdTop(0.00f, 0.46f, 0.75f);
    glm::vec3 fwdSide(0.72f, 0.11f, 0.75f);
    glm::vec3 fwdBot(0.00f, -0.38f, 0.75f);
    // Main hull mid ring (widest point)
    glm::vec3 midTop(0.00f, 0.40f, 0.00f);
    glm::vec3 midSide(0.84f, 0.07f, 0.00f);
    glm::vec3 midBot(0.00f, -0.42f, 0.00f);
    // Tail ring
    glm::vec3 tailTop(0.00f, 0.28f, -0.72f);
    glm::vec3 tailSide(0.58f, 0.03f, -0.72f);
    glm::vec3 tailBot(0.00f, -0.30f, -0.72f);
    // Nozzle ring (back)
    glm::vec3 nozzTop(0.00f, 0.22f, -1.20f);
    glm::vec3 nozzSide(0.44f, 0.00f, -1.20f);
    glm::vec3 nozzBot(0.00f, -0.22f, -1.20f);
    // Tail tip
    glm::vec3 tailTip(0.00f, 0.00f, -1.40f);

    // ── NOSE CONE ─────────────────────────────────────────────────────────────
    pushTri(noseTip, ckTop, ckSide);
    pushTri(noseTip, ckSide, ckBot);
    pushTri(noseTip, ckBot, { -ckSide.x,ckSide.y,ckSide.z });
    pushTri(noseTip, { -ckSide.x,ckSide.y,ckSide.z }, ckTop);

    // ── COCKPIT FAIRING — glass face (slightly concave front panel) ───────────
    glm::vec3 brow(0.00f, 0.66f, 1.22f);
    glm::vec3 browL(0.35f, 0.52f, 1.22f);
    // Glass panel (centre + both sides)
    pushTri(brow, browL, ckTop, { 0.5f,1 }, { 1,0 }, { 0,0 });
    pushTri(brow, ckTop, { -browL.x,browL.y,browL.z });
    // Lower glass sill
    pushTri(ckTop, browL, fwdSide, { 0,1 }, { 1,1 }, { 1,0 });
    pushTri(ckTop, fwdSide, { -fwdSide.x,fwdSide.y,fwdSide.z });

    // ── FUSELAGE SIDES (quads between rings) ──────────────────────────────────
    // Cockpit ring → forward ring (upper)
    pushQuadMirrorX(ckTop, { -ckTop.x,ckTop.y,ckTop.z },
        fwdTop, { -fwdTop.x,fwdTop.y,fwdTop.z });
    // Side
    pushQuadMirrorX(ckSide, { ckTop.x,ckTop.y,ckTop.z },
        fwdSide, fwdTop);
    // Lower
    pushQuadMirrorX(ckBot, ckSide, fwdBot, fwdSide);
    pushTri(ckTop, { -ckTop.x,ckTop.y,ckTop.z }, ckBot);   // forward bulkhead centre
    pushTri(fwdTop, fwdBot, { -fwdTop.x,fwdTop.y,fwdTop.z });

    // Forward ring → mid ring
    pushQuadMirrorX(fwdTop, { -fwdTop.x,fwdTop.y,fwdTop.z },
        midTop, { -midTop.x,midTop.y,midTop.z });
    pushQuadMirrorX(fwdSide, fwdTop, midSide, midTop);
    pushQuadMirrorX(fwdBot, fwdSide, midBot, midSide);

    // Mid ring → tail ring
    pushQuadMirrorX(midTop, { -midTop.x,midTop.y,midTop.z },
        tailTop, { -tailTop.x,tailTop.y,tailTop.z });
    pushQuadMirrorX(midSide, midTop, tailSide, tailTop);
    pushQuadMirrorX(midBot, midSide, tailBot, tailSide);

    // Tail ring → nozzle ring
    pushQuadMirrorX(tailTop, { -tailTop.x,tailTop.y,tailTop.z },
        nozzTop, { -nozzTop.x,nozzTop.y,nozzTop.z });
    pushQuadMirrorX(tailSide, tailTop, nozzSide, nozzTop);
    pushQuadMirrorX(tailBot, tailSide, nozzBot, nozzSide);

    // Nozzle ring → tail tip
    pushTri(nozzTop, { -nozzTop.x,nozzTop.y,nozzTop.z }, tailTip);
    pushTriMirrorX(nozzTop, nozzSide, tailTip);
    pushTriMirrorX(nozzSide, nozzBot, tailTip);
    pushTri(nozzBot, { -nozzBot.x,nozzBot.y,nozzBot.z }, tailTip);

    // Bottom fuselage close-off (belly panels)
    pushQuadMirrorX(fwdBot, { -fwdBot.x,fwdBot.y,fwdBot.z },
        midBot, { -midBot.x,midBot.y,midBot.z });
    pushQuadMirrorX(midBot, { -midBot.x,midBot.y,midBot.z },
        tailBot, { -tailBot.x,tailBot.y,tailBot.z });

    // ── SWEPT DELTA WINGS (scaled to match shorter fuselage) ─────────────────
    glm::vec3 wLeadRoot(0.70f, -0.11f, 0.80f);   // wing meets fuselage fwd
    glm::vec3 wLeadTip(2.60f, -0.20f, -0.10f);   // wide swept tip
    glm::vec3 wTrailRoot(0.58f, -0.14f, -0.65f);  // trailing edge at fuselage
    glm::vec3 wTrailTip(1.90f, -0.24f, -0.80f);  // trailing tip

    // Top face (flat upper wing surface)
    glm::vec3 wTop = { 0, 0.02f, 0 };  // slight upward bias for dihedral
    pushTriMirrorX(wLeadRoot + wTop, wLeadTip + wTop, wTrailTip + wTop);
    pushTriMirrorX(wLeadRoot + wTop, wTrailTip + wTop, wTrailRoot + wTop);

    // Bottom face (mirror winding)
    glm::vec3 wBot = { 0, -0.01f, 0 };
    pushTriMirrorX(wLeadTip + wBot, wLeadRoot + wBot, wTrailTip + wBot);
    pushTriMirrorX(wTrailTip + wBot, wLeadRoot + wBot, wTrailRoot + wBot);

    // Leading edge (angled face — sharp aerodynamic edge)
    pushTriMirrorX(wLeadRoot + wTop, wLeadTip + wTop, wLeadTip + wBot);
    pushTriMirrorX(wLeadRoot + wTop, wLeadTip + wBot, wLeadRoot + wBot);

    // Trailing edge
    pushTriMirrorX(wTrailRoot + wTop, wTrailTip + wBot, wTrailTip + wTop);
    pushTriMirrorX(wTrailRoot + wTop, wTrailRoot + wBot, wTrailTip + wBot);

    // Wing tip edge cap
    pushTriMirrorX(wLeadTip + wTop, wTrailTip + wTop, wTrailTip + wBot);
    pushTriMirrorX(wLeadTip + wTop, wTrailTip + wBot, wLeadTip + wBot);

    // ── WING CANARD FINS (small forward control surfaces) ─────────────────────
    glm::vec3 cRoot(0.36f, -0.06f, 1.30f);
    glm::vec3 cTip(0.82f, -0.10f, 0.90f);
    glm::vec3 cTrail(0.30f, -0.08f, 0.96f);
    glm::vec3 cOff = { 0,  0.03f, 0 };
    pushTriMirrorX(cRoot + cOff, cTip + cOff, cTrail + cOff);
    pushTriMirrorX(cTip - cOff, cRoot - cOff, cTrail - cOff);
    pushTriMirrorX(cRoot + cOff, cTip + cOff, cTip - cOff);
    pushTriMirrorX(cRoot + cOff, cTip - cOff, cRoot - cOff);

    // ── ENGINE PODS (under tail, one per side) ────────────────────────────────
    // Each pod is a simple hex tube with a flat nozzle cap
    auto buildPod = [&](float podX)
        {
            float sign = (podX > 0) ? 1.0f : -1.0f;

            // Pod cross-section: hexagonal, two rings
            const int PSIDES = 6;
            float PR = 0.24f;   // pod radius — thicker pods
            float PZ0 = -0.35f; // pod start Z (under wing join)
            float PZ1 = -1.35f; // pod end Z (nozzle) — shorter
            float PY = -0.40f; // pod Y offset (below fuselage)

            std::vector<glm::vec3> ring0, ring1;
            for (int i = 0; i < PSIDES; ++i) {
                float a = (float)i / PSIDES * 6.28318f;
                ring0.push_back({ podX + sign * cosf(a) * PR, PY + sinf(a) * PR * 0.7f, PZ0 });
                ring1.push_back({ podX + sign * cosf(a) * PR, PY + sinf(a) * PR * 0.7f, PZ1 });
            }

            // Side panels
            for (int i = 0; i < PSIDES; ++i) {
                int j = (i + 1) % PSIDES;
                pushQuad(ring0[i], ring0[j], ring1[i], ring1[j]);
            }

            // Front cap (dome-ish — point toward forward)
            glm::vec3 podNose = { podX, PY, PZ0 + 0.20f };
            for (int i = 0; i < PSIDES; ++i) {
                int j = (i + 1) % PSIDES;
                pushTri(podNose, ring0[j], ring0[i]);
            }

            // Nozzle cap (flat back face, slightly recessed)
            glm::vec3 nozzleCentre = { podX, PY, PZ1 - 0.04f };
            for (int i = 0; i < PSIDES; ++i) {
                int j = (i + 1) % PSIDES;
                pushTri(nozzleCentre, ring1[i], ring1[j]);
            }
        };

    buildPod(0.60f);
    buildPod(-0.60f);

    // ── DORSAL FINS ───────────────────────────────────────────────────────────
    glm::vec3 dBase0(0.06f, 0.38f, 0.35f);
    glm::vec3 dBase1(0.06f, 0.28f, -0.25f);
    glm::vec3 dTip0(0.06f, 0.75f, 0.10f);

    // Right fin panel
    pushTri(dBase0, dBase1, dTip0);
    pushTri(dBase1, dBase0, dTip0);  // back face

    // Left fin mirror
    pushTri({ -dBase0.x,dBase0.y,dBase0.z }, dTip0, { -dBase1.x,dBase1.y,dBase1.z });
    pushTri({ -dBase1.x,dBase1.y,dBase1.z }, dTip0, { -dBase0.x,dBase0.y,dBase0.z });

    // ── Upload to GPU ──────────────────────────────────────────────────────────
    shipVertCount = (int)verts.size() / 14;

    glGenVertexArrays(1, &shipVAO);
    glGenBuffers(1, &shipVBO);
    glBindVertexArray(shipVAO);
    glBindBuffer(GL_ARRAY_BUFFER, shipVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    const GLsizei stride = 14 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glBindVertexArray(0);

    std::cout << "[Ship] Hull mesh built: " << shipVertCount << " verts.\n";
}

// ─── buildTetherGeometry ─────────────────────────────────────────────────────
// Tether VAO: line-strip with ROPE_SEGMENTS+1 positions (3 floats each)
void SceneBasic_Uniform::buildTetherGeometry()
{
    glGenVertexArrays(1, &tetherVAO);
    glGenBuffers(1, &tetherVBO);

    glBindVertexArray(tetherVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tetherVBO);

    // Allocate space for ROPE_SEGMENTS+1 vec3 positions (stream update every frame)
    glBufferData(GL_ARRAY_BUFFER, (ROPE_SEGMENTS + 1) * 3 * sizeof(float), nullptr, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ─── setupSkyboxGeometry ─────────────────────────────────────────────────────
void SceneBasic_Uniform::setupSkyboxGeometry()
{
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_skyboxVerts), s_skyboxVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ─── setupScreenQuad ─────────────────────────────────────────────────────────
void SceneBasic_Uniform::setupScreenQuad()
{
    float v[] = {
        -1,  1, 0, 1,
        -1, -1, 0, 0,
         1, -1, 1, 0,
        -1,  1, 0, 1,
         1, -1, 1, 0,
         1,  1, 1, 1
    };

    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);

    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ─── setupHDRFramebuffer ─────────────────────────────────────────────────────
void SceneBasic_Uniform::setupHDRFramebuffer()
{
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(1, &hdrColorBuf);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuf, 0);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[HDR] Framebuffer incomplete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[BLUR] Ping-pong framebuffer incomplete: " << i << "\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─── loadCubemap ─────────────────────────────────────────────────────────────
GLuint SceneBasic_Uniform::loadCubemap(std::vector<std::string> faces)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++) {
        int w, h, ch;
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
        if (data) {
            GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "[Cubemap] Failed: " << faces[i] << "\n";
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return id;
}

// ─── loadTexture2D ───────────────────────────────────────────────────────────
GLuint SceneBasic_Uniform::loadTexture2D(const std::string& path, bool flip)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(flip);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);

    if (!data) {
        std::cerr << "[Texture] Failed: " << path << "\n";
        glDeleteTextures(1, &tex);
        return 0;
    }

    GLenum fmt = GL_RGB;
    if (ch == 1) fmt = GL_RED;
    else if (ch == 3) fmt = GL_RGB;
    else if (ch == 4) fmt = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    return tex;
}

void SceneBasic_Uniform::setupShadowMap()
{
    // ── Create depth texture ──────────────────────────────────────────────────
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Clamp to border with depth = 1 so areas outside the frustum aren't shadowed
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderCol[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderCol);

    // LearnOpenGL §Shadow Mapping: use GL_NEAREST for manual PCF.
    // GL_LINEAR bilinearly blends 4 neighbouring depth texels before returning
    // the value, so texture(...).r gives a blended depth that doesn't represent
    // any real surface.  Comparing currentDepth against that interpolated value
    // produces wrong shadow results near edges — shadows appear on lit inside
    // faces and "rotate" with the camera.  GL_NEAREST returns the exact stored
    // depth for each sample, which is what manual PCF requires.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // ── Create depth-only FBO ─────────────────────────────────────────────────
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, shadowMap, 0);

    // No colour attachment needed — depth only
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[Shadow] Depth FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Light-space matrix is computed dynamically each frame in update() so the
    // shadow source can wobble around the black hole.  Only the FBO/texture are
    // set up here.
    std::cout << "[Shadow] " << SHADOW_MAP_SIZE << "x" << SHADOW_MAP_SIZE
        << " PCF shadow map ready — light source at BH, matrix updated per-frame.\n";
}

void SceneBasic_Uniform::renderShadowPass()
{
    // lightSpaceMatrix is computed once in setupShadowMap() — the light
    // position and scene centre are static, so there is nothing to recompute.

    // ── Render depth pass ─────────────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);

    // LearnOpenGL §Shadow Mapping — peter-panning fix: cull FRONT faces so only
    // back-faces write into the depth map.  The platform is a closed hex prism
    // (top cap + side walls + bottom cap).  GL_FRONT removes the top/near-side
    // faces; the bottom cap and far-side walls remain.  Those back-face depths
    // are always > the corresponding front-face depths, so the front-face shadow
    // test (currentDepth < closestDepth) always passes cleanly — no self-shadow,
    // no need for a large bias.  Beacon-tower spheres work the same way: the back
    // hemisphere's depth is > any floor point below the tower, so floors/decks
    // under the tower correctly receive the tower's shadow.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    shadowProg.use();
    shadowProg.setUniform("uLightSpaceMatrix", lightSpaceMatrix);

    // ── Draw all shadow-casting geometry ──────────────────────────────────────
    // Main home platform (upper) — pristine, no dissolve
    shadowProg.setUniform("uDissolve", 0.0f);
    glm::mat4 upperModel =
        glm::translate(glm::mat4(1.f), glm::vec3(0, 0.8f, 145.0f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(1.9f, 1.f, 1.9f));
    shadowProg.setUniform("Model", upperModel);
    glBindVertexArray(platVAO);
    glDrawArrays(GL_TRIANGLES, 0, platVertCount);

    // Main home platform (lower)
    glm::mat4 lowerModel =
        glm::translate(glm::mat4(1.f), glm::vec3(0, -1.1f, 145.0f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(2.35f, 1.f, 2.35f));
    shadowProg.setUniform("Model", lowerModel);
    glDrawArrays(GL_TRIANGLES, 0, platVertCount);

    // Satellite relay platforms — dissolve matches main render pass (Week 9)
    for (const auto& zone : platformZones)
    {
        float shadowDissolve = 0.0f;
        if (zone.type == PlatformZoneType::DamagedRelay) shadowDissolve = 0.13f;
        else if (zone.type == PlatformZoneType::HazardRelay) shadowDissolve = 0.22f;
        shadowProg.setUniform("uDissolve", shadowDissolve);

        glm::mat4 satModel =
            glm::translate(glm::mat4(1.f), zone.position) *
            glm::scale(glm::mat4(1.f), glm::vec3(0.75f, 1.f, 0.75f));
        shadowProg.setUniform("Model", satModel);
        glDrawArrays(GL_TRIANGLES, 0, platVertCount);
    }

    // Beacon towers — tall vertical structures on every platform.
    // These are the MOST VISIBLE shadow casters: at 18° light elevation a
    // beacon 4 units tall casts a shadow ~12 units long across the deck.
    // The sphere VAO is reused as a simple stand-in for the beacon body.
    shadowProg.setUniform("uDissolve", 0.0f);   // beacons never dissolve
    glBindVertexArray(sphereVAO);
    for (const auto& beacon : beaconTowers)
    {
        // Match the beacon's world transform used in renderBeaconTowers()
        glm::mat4 bModel = glm::translate(glm::mat4(1.f),
                               beacon.position + glm::vec3(0.0f, beacon.scale.y * 0.5f, 0.0f))
                         * glm::scale(glm::mat4(1.f), beacon.scale);
        shadowProg.setUniform("Model", bModel);
        glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
    }
    // Asteroids cast shadows too (large ones near camera)
    for (const auto& asteroid : asteroids)
    {
        if (!asteroid.active) continue;
        // Only cast shadow if close enough to matter
        float distCam = glm::length(asteroid.position - camera.Position);
        if (distCam > 600.0f) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, asteroid.position);
        model = glm::rotate(model, glm::radians(asteroid.rotationAngle), asteroid.rotationAxis);
        model = glm::scale(model, asteroid.scale);
        shadowProg.setUniform("Model", model);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIdxCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);

    // Restore normal back-face culling
    glCullFace(GL_BACK);

    // Restore main framebuffer + viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

void SceneBasic_Uniform::setShadowUniforms()
{
    // Bind shadow map to texture unit 5
    // (units 0-4 used by platform/cell textures)
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    platformProg.use();
    platformProg.setUniform("uShadowMap", 5);
    platformProg.setUniform("uLightSpaceMatrix", lightSpaceMatrix);
    platformProg.setUniform("uShadowEnabled", shadowsEnabled ? 1 : 0);
}

// ─── loadOBJMesh ─────────────────────────────────────────────────────────────
// Self-contained Wavefront OBJ loader — zero external dependencies.
// Reads vertex positions (v), texture coordinates (vt), normals (vn),
// and face indices (f) via std::ifstream.  Handles all OBJ face formats:
//   v     (position only)
//   v/vt  (position + texcoord)
//   v//vn (position + normal, no texcoord)
//   v/vt/vn (full)
// Quads and n-gons are triangulated by fan decomposition.
// Per-triangle tangent and bitangent vectors are computed from UV deltas
// using the standard tangent-space derivation (Lengyel 2001).
// The resulting VAO uses the same 14-float stride as sphereVAO/platVAO
// (pos3 | norm3 | uv2 | tan3 | bitan3), so any Blender .obj export drops
// straight into platformProg without shader changes.
//
// Reference: Wavefront OBJ format specification (Wavefront Technologies, 1992)
//            Lengyel, E. (2001). "Computing Tangent Space Basis Vectors."
//            Terathon Software — tangent/bitangent derivation formula.
SceneBasic_Uniform::OBJMesh SceneBasic_Uniform::loadOBJMesh(const std::string& path)
{
    OBJMesh result;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[OBJ] Cannot open: " << path << "\n";
        return result;
    }

    std::vector<glm::vec3> rawPos;
    std::vector<glm::vec2> rawUV;
    std::vector<glm::vec3> rawNorm;

    // Face vertex: 0-based indices; -1 = attribute not present
    struct FV { int p, t, n; };
    std::vector<FV> faceVerts;  // 3 entries per triangle after fan-split

    // Parse one face token (e.g. "3/2/1", "5//3", "7")
    auto parseFV = [](const std::string& tok) -> FV {
        FV fv{ -1, -1, -1 };
        std::string parts[3];
        int pi = 0;
        for (char c : tok) {
            if (c == '/') { if (++pi > 2) break; }
            else           parts[pi] += c;
        }
        if (!parts[0].empty()) fv.p = std::stoi(parts[0]) - 1;
        if (!parts[1].empty()) fv.t = std::stoi(parts[1]) - 1;
        if (!parts[2].empty()) fv.n = std::stoi(parts[2]) - 1;
        return fv;
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tok;
        ss >> tok;

        if (tok == "v") {
            float x, y, z; ss >> x >> y >> z;
            rawPos.push_back({ x, y, z });
        }
        else if (tok == "vt") {
            float u, v; ss >> u >> v;
            rawUV.push_back({ u, v });
        }
        else if (tok == "vn") {
            float x, y, z; ss >> x >> y >> z;
            rawNorm.push_back({ x, y, z });
        }
        else if (tok == "f") {
            std::vector<FV> poly;
            std::string vtok;
            while (ss >> vtok) poly.push_back(parseFV(vtok));
            // Fan triangulation: (0,1,2), (0,2,3), (0,3,4) ...
            for (int i = 1; i + 1 < (int)poly.size(); ++i) {
                faceVerts.push_back(poly[0]);
                faceVerts.push_back(poly[i]);
                faceVerts.push_back(poly[i + 1]);
            }
        }
        // 'o', 'g', 'mtllib', 'usemtl', 's' — ignored (geometry only)
    }

    if (faceVerts.empty()) {
        std::cerr << "[OBJ] No face data parsed in: " << path << "\n";
        return result;
    }

    bool hasNorm = !rawNorm.empty();
    bool hasUV   = !rawUV.empty();

    // Build interleaved vertex + index buffers
    std::vector<float>        verts;
    std::vector<unsigned int> indices;
    std::map<std::tuple<int,int,int>, unsigned int> seen;

    for (int tri = 0; tri < (int)faceVerts.size(); tri += 3)
    {
        // Fetch positions and UVs for this triangle
        glm::vec3 p[3]; glm::vec2 uv[3];
        for (int k = 0; k < 3; ++k) {
            int pi = faceVerts[tri+k].p;
            int ti = faceVerts[tri+k].t;
            p[k]  = (pi >= 0 && pi < (int)rawPos.size()) ? rawPos[pi] : glm::vec3(0);
            // Fallback UVs: simple per-vertex constants so degenerate det doesn't matter
            uv[k] = (ti >= 0 && ti < (int)rawUV.size())
                  ? rawUV[ti]
                  : glm::vec2(k == 1 ? 1.0f : 0.0f, k == 2 ? 1.0f : 0.0f);
        }

        // Tangent / bitangent from edge + UV deltas (Lengyel 2001)
        glm::vec3 e1 = p[1] - p[0], e2 = p[2] - p[0];
        glm::vec2 d1 = uv[1] - uv[0], d2 = uv[2] - uv[0];
        float det = d1.x * d2.y - d2.x * d1.y;

        glm::vec3 tangent(1, 0, 0), bitangent(0, 1, 0);
        if (fabsf(det) > 1e-6f) {
            float inv = 1.0f / det;
            tangent   = glm::normalize((e1 *  d2.y - e2 *  d1.y) * inv);
            bitangent = glm::normalize((e2 *  d1.x - e1 *  d2.x) * inv);
        } else {
            // Degenerate UVs — use edge direction as tangent
            if (glm::length(e1) > 1e-6f) tangent = glm::normalize(e1);
            glm::vec3 fn = glm::cross(e1, e2);
            if (glm::length(fn) > 1e-6f)
                bitangent = glm::normalize(glm::cross(glm::normalize(fn), tangent));
        }

        // Face normal (used if OBJ has no vn entries)
        glm::vec3 faceNorm(0, 1, 0);
        if (!hasNorm) {
            glm::vec3 fn = glm::cross(e1, e2);
            if (glm::length(fn) > 1e-6f) faceNorm = glm::normalize(fn);
        }

        for (int k = 0; k < 3; ++k) {
            const FV& fv = faceVerts[tri + k];
            auto key = std::make_tuple(fv.p, fv.t, fv.n);

            auto it = seen.find(key);
            if (it != seen.end()) {
                indices.push_back(it->second);
            }
            else {
                unsigned int idx = (unsigned int)(verts.size() / 14);
                seen[key] = idx;
                indices.push_back(idx);

                glm::vec3 pos = (fv.p >= 0 && fv.p < (int)rawPos.size())
                                ? rawPos[fv.p] : glm::vec3(0);
                glm::vec3 nor = (hasNorm && fv.n >= 0 && fv.n < (int)rawNorm.size())
                                ? rawNorm[fv.n] : faceNorm;
                glm::vec2 tex = (hasUV && fv.t >= 0 && fv.t < (int)rawUV.size())
                                ? rawUV[fv.t] : glm::vec2(0);

                verts.insert(verts.end(), {
                    pos.x, pos.y, pos.z,
                    nor.x, nor.y, nor.z,
                    tex.x, tex.y,
                    tangent.x,   tangent.y,   tangent.z,
                    bitangent.x, bitangent.y, bitangent.z
                });
            }
        }
    }

    if (verts.empty()) {
        std::cerr << "[OBJ] Geometry empty after processing: " << path << "\n";
        return result;
    }

    // ── Upload to GPU (same layout as sphereVAO) ──────────────────────────────
    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    glBindVertexArray(result.vao);

    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = 14 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3  * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6  * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8  * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    result.indexCount = (int)indices.size();
    result.loaded = true;

    std::cout << "[OBJ] Loaded: " << path
              << " — " << (verts.size() / 14) << " verts, "
              << (indices.size() / 3) << " tris.\n";
    return result;
}

// ─── Particle System (Week 7 — Instanced Particles + Week 9 — Noise) ─────────
// 2000 billboard quads spiral toward the black hole driven by FBM noise.
// CPU side: manages particle lifetimes and streams per-instance data.
// GPU side: vertex shader animates position using Schwarzschild-inspired
//           gravity + Perlin FBM turbulence (see shader/particles.vert).
//
// Reference: GPU Gems 3, Ch.23 (instancing pattern);
//            Perlin (1985) noise basis used in particles.vert ptFbm().
