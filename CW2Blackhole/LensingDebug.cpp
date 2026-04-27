// LensingDebug.cpp  –  modern OpenGL 3.3 core rewrite
// Every legacy call (glBegin/glEnd/glVertex/glOrtho/glMatrixMode/glColor4f etc.)
// has been replaced with a VAO + VBO + inline GLSL approach.

#include "LensingDebug.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

// ??? Inline GLSL ?????????????????????????????????????????????????????????????
static const char* s_vert = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)GLSL";

static const char* s_frag = R"GLSL(
#version 330 core
in  vec4 vColor;
out vec4 FragColor;
void main()
{
    FragColor = vColor;
}
)GLSL";

// ??? Bitmap glyphs (3×5, same table as scenebasic_uniform.cpp) ???????????????
const char* LensingDebugSystem::getGlyph(char c)
{
    switch (c)
    {
    case 'A': return "010,101,111,101,101";
    case 'B': return "110,101,110,101,110";
    case 'C': return "011,100,100,100,011";
    case 'D': return "110,101,101,101,110";
    case 'E': return "111,100,110,100,111";
    case 'F': return "111,100,110,100,100";
    case 'G': return "011,100,101,101,011";
    case 'H': return "101,101,111,101,101";
    case 'I': return "111,010,010,010,111";
    case 'J': return "001,001,001,101,010";
    case 'K': return "101,101,110,101,101";
    case 'L': return "100,100,100,100,111";
    case 'M': return "101,111,111,101,101";
    case 'N': return "101,111,111,111,101";
    case 'O': return "111,101,101,101,111";
    case 'P': return "110,101,110,100,100";
    case 'Q': return "111,101,101,111,001";
    case 'R': return "110,101,110,101,101";
    case 'S': return "011,100,111,001,110";
    case 'T': return "111,010,010,010,010";
    case 'U': return "101,101,101,101,111";
    case 'V': return "101,101,101,101,010";
    case 'W': return "101,101,111,111,101";
    case 'X': return "101,101,010,101,101";
    case 'Y': return "101,101,010,010,010";
    case 'Z': return "111,001,010,100,111";
    case '0': return "111,101,101,101,111";
    case '1': return "010,110,010,010,111";
    case '2': return "111,001,111,100,111";
    case '3': return "111,001,111,001,111";
    case '4': return "101,101,111,001,001";
    case '5': return "111,100,111,001,111";
    case '6': return "011,100,111,101,111";
    case '7': return "111,001,001,001,001";
    case '8': return "111,101,111,101,111";
    case '9': return "111,101,111,001,110";
    case ':': return "000,010,000,010,000";
    case '/': return "001,001,010,100,100";
    case '-': return "000,000,111,000,000";
    case '.': return "000,000,000,000,010";
    case '%': return "101,001,010,100,101";
    case '!': return "010,010,010,000,010";
    default:  return "000,000,000,000,000";
    }
}

bool LensingDebugSystem::glyphPixel(const char* glyph, int row, int col)
{
    int idx = row * 4 + col;   // each row is "XYZ," (4 chars)
    return glyph[idx] == '1';
}

// ??? Constructor / Destructor ?????????????????????????????????????????????????
LensingDebugSystem::LensingDebugSystem() {}
LensingDebugSystem::~LensingDebugSystem() { destroyGPU(); }

// ??? reset ???????????????????????????????????????????????????????????????????
void LensingDebugSystem::reset()
{
    m_time = 0.0f;
    m_ringSpin = 0.0f;
    m_einsteinRad = 0.35f;
    m_sourceDist = 0.55f;
    m_lensMass = 0.70f;
}

// ??? update ??????????????????????????????????????????????????????????????????
void LensingDebugSystem::update(float dt)
{
    m_time += dt;
    m_ringSpin += dt * 0.22f;

    // Gentle parameter oscillation for visual interest
    m_einsteinRad = 0.30f + 0.08f * sinf(m_time * 0.31f);
    m_sourceDist = 0.50f + 0.12f * sinf(m_time * 0.19f);
    m_lensMass = 0.65f + 0.18f * sinf(m_time * 0.14f);
}

// ??? handleInput ?????????????????????????????????????????????????????????????
void LensingDebugSystem::handleInput(GLFWwindow* window)
{
    bool lNow = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lNow && !m_lPrev)
    {
        m_visible = !m_visible;
        std::cout << "[LensingDebug] Panel " << (m_visible ? "ON" : "OFF") << "\n";
    }
    m_lPrev = lNow;
}

// ??? GPU init/destroy ?????????????????????????????????????????????????????????
void LensingDebugSystem::initGPU()
{
    if (m_gpuReady) return;

    // Compile vertex shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &s_vert, nullptr);
    glCompileShader(vs);
    {
        GLint ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512]; glGetShaderInfoLog(vs, 512, nullptr, buf);
            std::cerr << "[LensingDebug] VS error: " << buf << "\n";
        }
    }

    // Compile fragment shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &s_frag, nullptr);
    glCompileShader(fs);
    {
        GLint ok; glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512]; glGetShaderInfoLog(fs, 512, nullptr, buf);
            std::cerr << "[LensingDebug] FS error: " << buf << "\n";
        }
    }

    // Link program
    m_shader = glCreateProgram();
    glAttachShader(m_shader, vs);
    glAttachShader(m_shader, fs);
    glLinkProgram(m_shader);
    {
        GLint ok; glGetProgramiv(m_shader, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[512]; glGetProgramInfoLog(m_shader, 512, nullptr, buf);
            std::cerr << "[LensingDebug] Link error: " << buf << "\n";
        }
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // VAO/VBO (dynamic, updated each draw call)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // layout(location=0) = vec2 pos,  layout(location=1) = vec4 color
    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    m_gpuReady = true;
}

void LensingDebugSystem::destroyGPU()
{
    if (!m_gpuReady) return;
    glDeleteProgram(m_shader);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    m_gpuReady = false;
}

// ??? flushTris ???????????????????????????????????????????????????????????????
// verts layout: x, y, r, g, b, a   (6 floats per vertex, 3 verts per triangle)
void LensingDebugSystem::flushTris(const std::vector<float>& verts)
{
    if (verts.empty()) return;

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 6));
    glBindVertexArray(0);
}

// ??? Primitive helpers ????????????????????????????????????????????????????????
void LensingDebugSystem::drawRect(float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    if (w <= 0.0f || h <= 0.0f) return;

    // Convert pixel coords to NDC
    float x0 = toNDCx(x), y0 = toNDCy(y);
    float x1 = toNDCx(x + w), y1 = toNDCy(y + h);

    // Two triangles (counter-clockwise)
    std::vector<float> v = {
        x0, y0,  r,g,b,a,
        x1, y0,  r,g,b,a,
        x1, y1,  r,g,b,a,

        x0, y0,  r,g,b,a,
        x1, y1,  r,g,b,a,
        x0, y1,  r,g,b,a,
    };
    flushTris(v);
}

void LensingDebugSystem::drawCircle(float cx, float cy, float radius,
    float r, float g, float b, float a,
    int segments, float thickness)
{
    // Build a ring of thin quads
    std::vector<float> v;
    v.reserve(segments * 6 * 6);

    const float pi2 = 6.28318530f;
    for (int i = 0; i < segments; ++i)
    {
        float a0 = pi2 * (float)i / segments;
        float a1 = pi2 * (float)(i + 1) / segments;

        float c0x = cx + cosf(a0) * radius, c0y = cy + sinf(a0) * radius;
        float c1x = cx + cosf(a1) * radius, c1y = cy + sinf(a1) * radius;
        float i0x = cx + cosf(a0) * (radius - thickness);
        float i0y = cy + sinf(a0) * (radius - thickness);
        float i1x = cx + cosf(a1) * (radius - thickness);
        float i1y = cy + sinf(a1) * (radius - thickness);

        auto nx0 = toNDCx(c0x), ny0 = toNDCy(c0y);
        auto nx1 = toNDCx(c1x), ny1 = toNDCy(c1y);
        auto ni0 = toNDCx(i0x), nj0 = toNDCy(i0y);
        auto ni1 = toNDCx(i1x), nj1 = toNDCy(i1y);

        v.insert(v.end(), {
            nx0, ny0, r,g,b,a,
            nx1, ny1, r,g,b,a,
            ni1, nj1, r,g,b,a,
            nx0, ny0, r,g,b,a,
            ni1, nj1, r,g,b,a,
            ni0, nj0, r,g,b,a,
            });
    }
    flushTris(v);
}

void LensingDebugSystem::drawDisk(float cx, float cy, float radius,
    float r, float g, float b, float a,
    int segments)
{
    std::vector<float> v;
    v.reserve(segments * 3 * 6);

    const float pi2 = 6.28318530f;
    float ncx = toNDCx(cx), ncy = toNDCy(cy);

    for (int i = 0; i < segments; ++i)
    {
        float a0 = pi2 * (float)i / segments;
        float a1 = pi2 * (float)(i + 1) / segments;

        float nx0 = toNDCx(cx + cosf(a0) * radius);
        float ny0 = toNDCy(cy + sinf(a0) * radius);
        float nx1 = toNDCx(cx + cosf(a1) * radius);
        float ny1 = toNDCy(cy + sinf(a1) * radius);

        v.insert(v.end(), {
            ncx, ncy, r,g,b,a,
            nx0, ny0, r,g,b,a,
            nx1, ny1, r,g,b,a,
            });
    }
    flushTris(v);
}

void LensingDebugSystem::drawBar(float x, float y, float w, float h,
    float value,
    float fr, float fg, float fb)
{
    // Background
    drawRect(x, y, w, h, 0.04f, 0.06f, 0.10f, 0.85f);
    // Border
    drawRect(x, y, w, 1.5f, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(x, y + h, w, 1.5f, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(x, y, 1.5f, h, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(x + w, y, 1.5f, h, 0.18f, 0.82f, 1.00f, 1.0f);
    // Fill
    float fillW = (w - 4.0f) * glm::clamp(value, 0.0f, 1.0f);
    if (fillW > 0.0f)
        drawRect(x + 2.0f, y + 2.0f, fillW, h - 4.0f, fr, fg, fb, 1.0f);
}

void LensingDebugSystem::drawText(float x, float y, const std::string& str,
    float scale,
    float r, float g, float b, float a)
{
    float pen = x;
    for (char ch : str)
    {
        if (ch == ' ') { pen += scale * 4.0f; continue; }
        const char* glyph = getGlyph((char)toupper((unsigned char)ch));
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col)
                if (glyphPixel(glyph, row, col))
                    drawRect(pen + col * scale,
                        y + (4 - row) * scale,
                        scale, scale, r, g, b, a);
        pen += scale * 4.0f;
    }
}

// ??? draw ?????????????????????????????????????????????????????????????????????
void LensingDebugSystem::draw(int screenW, int screenH)
{
    if (!m_visible) return;

    m_W = screenW;
    m_H = screenH;

    initGPU();  // no-op after first call

    // Save GL state
    GLboolean depthTest; glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    GLboolean blend;     glGetBooleanv(GL_BLEND, &blend);
    GLboolean scissor;   glGetBooleanv(GL_SCISSOR_TEST, &scissor);
    GLboolean cullFace;  glGetBooleanv(GL_CULL_FACE, &cullFace);
    GLint     viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_shader);
    glViewport(0, 0, screenW, screenH);

    // ?? Panel geometry ????????????????????????????????????????????????????????
    const float PW = 320.0f;  // panel width
    const float PH = 340.0f;  // panel height
    const float PX = (float)screenW - PW - 14.0f;  // right-aligned
    const float PY = 14.0f;                         // from bottom

    // Dark background
    drawRect(PX, PY, PW, PH, 0.02f, 0.03f, 0.06f, 0.88f);

    // Cyan border
    drawRect(PX, PY, PW, 2.0f, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(PX, PY + PH - 2, PW, 2.0f, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(PX, PY, 2.0f, PH, 0.18f, 0.82f, 1.00f, 1.0f);
    drawRect(PX + PW - 2, PY, 2.0f, PH, 0.18f, 0.82f, 1.00f, 1.0f);

    // Title bar
    drawRect(PX + 2, PY + PH - 24, PW - 4, 22, 0.06f, 0.28f, 0.60f, 0.90f);
    drawText(PX + 8, PY + PH - 20, "GRAVITATIONAL LENSING", 2.5f,
        0.80f, 0.96f, 1.00f, 1.0f);

    // ?? Simulation viewport (mini Einstein ring diagram) ???????????????????????
    float vizCX = PX + PW * 0.50f;
    float vizCY = PY + PH * 0.50f + 10.0f;
    float vizR = 100.0f;

    // Background circle
    drawDisk(vizCX, vizCY, vizR, 0.02f, 0.04f, 0.08f, 0.95f);
    drawCircle(vizCX, vizCY, vizR, 0.18f, 0.82f, 1.00f, 0.35f, 48, 1.5f);

    // Grid lines (horizontal + vertical thin rects)
    drawRect(vizCX - vizR, vizCY - 0.5f, vizR * 2.0f, 1.0f, 0.12f, 0.28f, 0.45f, 0.40f);
    drawRect(vizCX - 0.5f, vizCY - vizR, 1.0f, vizR * 2.0f, 0.12f, 0.28f, 0.45f, 0.40f);

    // Einstein ring (animated, radius driven by m_einsteinRad * vizR)
    float ringR = m_einsteinRad * vizR;
    float glow = 0.75f + 0.25f * sinf(m_time * 2.0f);
    drawCircle(vizCX, vizCY, ringR, 0.25f, 0.80f * glow, 1.40f * glow, 0.90f, 64, 2.5f);

    // Lens mass (bright core)
    float coreR = 6.0f + 3.0f * m_lensMass;
    drawDisk(vizCX, vizCY, coreR, 1.00f, 0.88f, 0.60f, 1.0f);

    // Source arcs (two symmetric arcs on either side)
    float srcDeg = 38.0f + 24.0f * m_sourceDist;
    const float PI = 3.14159265f;
    for (int side = 0; side < 2; ++side)
    {
        float baseAngle = m_ringSpin + (float)side * PI;
        std::vector<float> av;
        int arcSeg = 28;
        float arcHalf = (srcDeg / 360.0f) * 6.28318f * 0.5f;
        for (int i = 0; i < arcSeg; ++i)
        {
            float t0 = baseAngle - arcHalf + (float)i / arcSeg * arcHalf * 2.0f;
            float t1 = baseAngle - arcHalf + (float)(i + 1) / arcSeg * arcHalf * 2.0f;
            float aR = ringR * 1.18f;
            float iR = aR - 3.5f;

            float ox0 = toNDCx(vizCX + cosf(t0) * aR), oy0 = toNDCy(vizCY + sinf(t0) * aR);
            float ox1 = toNDCx(vizCX + cosf(t1) * aR), oy1 = toNDCy(vizCY + sinf(t1) * aR);
            float ix0 = toNDCx(vizCX + cosf(t0) * iR), iy0 = toNDCy(vizCY + sinf(t0) * iR);
            float ix1 = toNDCx(vizCX + cosf(t1) * iR), iy1 = toNDCy(vizCY + sinf(t1) * iR);

            av.insert(av.end(), {
                ox0,oy0, 0.35f,0.75f,1.00f, 0.85f,
                ox1,oy1, 0.35f,0.75f,1.00f, 0.85f,
                ix1,iy1, 0.35f,0.75f,1.00f, 0.85f,
                ox0,oy0, 0.35f,0.75f,1.00f, 0.85f,
                ix1,iy1, 0.35f,0.75f,1.00f, 0.85f,
                ix0,iy0, 0.35f,0.75f,1.00f, 0.85f,
                });
        }
        flushTris(av);
    }

    // ?? Parameter bars ????????????????????????????????????????????????????????
    float barX = PX + 12;
    float barW = PW - 24;
    float barH = 10.0f;
    float barY0 = PY + 14.0f;

    // Einstein Radius
    drawText(barX, barY0 + barH + 3, "EINSTEIN RADIUS", 2.0f, 0.55f, 0.90f, 1.00f, 1.0f);
    drawBar(barX, barY0, barW, barH, m_einsteinRad, 0.25f, 0.80f, 1.40f);

    // Source Distance
    drawText(barX, barY0 + barH * 3.5f + 3, "SOURCE DIST", 2.0f, 0.55f, 0.90f, 1.00f, 1.0f);
    drawBar(barX, barY0 + barH * 3.0f, barW, barH, m_sourceDist, 0.35f, 0.65f, 1.20f);

    // Lens Mass
    drawText(barX, barY0 + barH * 6.0f + 3, "LENS MASS", 2.0f, 0.55f, 0.90f, 1.00f, 1.0f);
    drawBar(barX, barY0 + barH * 5.5f, barW, barH, m_lensMass, 1.00f, 0.68f, 0.25f);

    // Footer hint
    drawText(PX + 8, PY + 6, "L KEY  TOGGLE", 2.0f, 0.38f, 0.60f, 0.80f, 0.70f);

    // ?? Restore GL state ??????????????????????????????????????????????????????
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    if (depthTest) glEnable(GL_DEPTH_TEST);
    if (cullFace)  glEnable(GL_CULL_FACE);
    if (!blend)    glDisable(GL_BLEND);
    if (scissor)   glEnable(GL_SCISSOR_TEST);
    glUseProgram(0);
}