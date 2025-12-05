#pragma once

#include <glad/glad.h>
#include <cstddef>

struct Mesh {
    GLuint vao, vbo, ebo;
    GLsizei indexCount;
    void destroy();
};

Mesh createSphere(int X_SEGMENTS, int Y_SEGMENTS);
Mesh createRing(float innerRadius, float outerRadius, int segments);
