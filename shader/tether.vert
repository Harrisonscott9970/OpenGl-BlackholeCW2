#version 330 core

// tether.vert — pass world-space position to the geometry shader
//
// The tether rope nodes are already in world space (updated by the
// Verlet/spring rope simulation each frame).  No model matrix is needed.
// The geometry shader applies view-projection.

layout(location = 0) in vec3 aPos;

out vec3 vWorldPos;

void main()
{
    vWorldPos   = aPos;
    // gl_Position is a placeholder — the geometry shader overwrites it.
    // Setting it here satisfies any driver that performs pre-GS culling.
    gl_Position = vec4(aPos, 1.0);
}
