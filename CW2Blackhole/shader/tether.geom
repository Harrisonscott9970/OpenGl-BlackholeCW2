#version 330 core

// tether.geom  —  Geometry shader: rope segment → camera-facing ribbon quad
//
// Week 6 (CW2 NEW):  Geometry Shader Stage
//
// Input:  GL_LINE_STRIP  →  the geometry shader receives each consecutive
//         pair of rope node positions as a 2-vertex line primitive.
//
// Output: GL_TRIANGLE_STRIP (4 vertices = 1 quad per segment)
//
// Each rope segment is extruded perpendicular to both the segment direction
// and the camera view vector, producing a flat ribbon that always faces the
// camera.  This "camera-facing tube" technique makes the rope visible from
// any angle and gives it a glowing volumetric appearance.
//
// Why a geometry shader is needed:
//   GL_LINES with fat glLineWidth is clamped to 1 px on modern OpenGL
//   core-profile drivers.  Achieving a visible, smooth, anti-aliased tube
//   requires generating actual billboard geometry — which can only be done
//   programmatically in a geometry shader.
//
// Technique reference:
//   Rost, R. et al. (2009). OpenGL Shading Language (3rd ed.), Ch. 12.
//   "Billboard ribbon" approach: per-segment camera-perpendicular offset.

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in  vec3 vWorldPos[];   // world-space endpoints from tether.vert

uniform mat4  uViewProj;   // proj * view
uniform vec3  uCamPos;     // world-space camera position
uniform float uHalfWidth;  // world-space ribbon half-width (default 0.18 m)

out vec2  vLocalUV;    // x = -1..1 across ribbon, y = 0..1 along segment
out float vSegAlpha;   // per-vertex alpha (reserved for future taper effects)

void main()
{
    vec3 p0 = vWorldPos[0];
    vec3 p1 = vWorldPos[1];

    // Compute the direction along the segment
    vec3  seg    = p1 - p0;
    float segLen = length(seg);
    if (segLen < 0.0001) return;   // degenerate / zero-length segment — skip
    seg /= segLen;

    // Camera-facing perpendicular:
    //   right = cross(seg, toEye) makes the ribbon face the viewer from any angle
    vec3 mid   = (p0 + p1) * 0.5;
    vec3 toEye = normalize(uCamPos - mid);
    vec3 right = normalize(cross(seg, toEye));

    vec3 off = right * uHalfWidth;

    // Emit 4 vertices as a quad (2 triangles via TRIANGLE_STRIP)
    gl_Position = uViewProj * vec4(p0 - off, 1.0);
    vLocalUV    = vec2(-1.0, 0.0);
    vSegAlpha   = 1.0;
    EmitVertex();

    gl_Position = uViewProj * vec4(p0 + off, 1.0);
    vLocalUV    = vec2( 1.0, 0.0);
    vSegAlpha   = 1.0;
    EmitVertex();

    gl_Position = uViewProj * vec4(p1 - off, 1.0);
    vLocalUV    = vec2(-1.0, 1.0);
    vSegAlpha   = 1.0;
    EmitVertex();

    gl_Position = uViewProj * vec4(p1 + off, 1.0);
    vLocalUV    = vec2( 1.0, 1.0);
    vSegAlpha   = 1.0;
    EmitVertex();

    EndPrimitive();
}
