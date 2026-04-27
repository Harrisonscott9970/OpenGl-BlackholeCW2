#pragma once
// LensingDebug.h  –  modern OpenGL 3.3 core rewrite
// Replaces the legacy glBegin/glVertex/glOrtho version that caused 40+ C3861 errors.
// All drawing is done with a minimal VAO+VBO pipeline and an inline GLSL program.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// ??? LensingDebugSystem ???????????????????????????????????????????????????????
class LensingDebugSystem
{
public:
    LensingDebugSystem();
    ~LensingDebugSystem();

    // Call once per frame before drawing
    void update(float dt);

    // Handle keyboard toggle (L key)
    void handleInput(GLFWwindow* window);

    // Draw the panel (call after all 3D rendering, before buffer swap)
    void draw(int screenW, int screenH);

    // Reset simulation state
    void reset();

private:
    // ?? State ??????????????????????????????????????????????????????????????????
    bool   m_visible = false;
    bool   m_lPrev = false;
    float  m_time = 0.0f;

    // Gravitational lensing ring simulation parameters
    float  m_einsteinRad = 0.35f;   // Einstein radius (normalised 0-1)
    float  m_sourceDist = 0.55f;   // Source distance slider
    float  m_lensMass = 0.70f;   // Lens mass slider
    float  m_ringSpin = 0.0f;    // Animated ring rotation

    // ?? GPU resources ?????????????????????????????????????????????????????????
    GLuint m_shader = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    bool   m_gpuReady = false;

    // ?? Helpers ???????????????????????????????????????????????????????????????
    void initGPU();
    void destroyGPU();

    // Upload a flat list of (x,y,r,g,b,a) vertices and draw them as triangles
    void flushTris(const std::vector<float>& verts);

    // Draw filled rectangle in screen-space [0,W]x[0,H] (y up from bottom)
    void drawRect(float x, float y, float w, float h,
        float r, float g, float b, float a = 1.0f);

    // Draw a circle outline using thin triangulated strips
    void drawCircle(float cx, float cy, float radius,
        float r, float g, float b, float a,
        int segments = 48, float thickness = 1.5f);

    // Draw a filled circle
    void drawDisk(float cx, float cy, float radius,
        float r, float g, float b, float a,
        int segments = 48);

    // Draw a horizontal progress bar
    void drawBar(float x, float y, float w, float h,
        float value,
        float fr, float fg, float fb);

    // Draw tiny 3x5 bitmap text
    void drawText(float x, float y, const std::string& str,
        float scale,
        float r, float g, float b, float a = 1.0f);

    // Pixel glyph helper
    static const char* getGlyph(char c);
    static bool        glyphPixel(const char* glyph, int row, int col);

    // Screen dimensions (set each draw call)
    int m_W = 1, m_H = 1;

    // Convert from "panel local" coords to NDC
    float toNDCx(float px) const { return  (px / m_W) * 2.0f - 1.0f; }
    float toNDCy(float py) const { return  (py / m_H) * 2.0f - 1.0f; }
};