#include "mesh.h"
#include <vector>
#include <cmath>
#include <glad/glad.h>

using namespace std;

Mesh createSphere(int X_SEGMENTS, int Y_SEGMENTS) {
    vector<float> data;
    vector<unsigned int> indices;
    for (int y = 0; y <= Y_SEGMENTS; ++y) {
        for (int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = cos(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);
            float yPos = cos(ySegment * M_PI);
            float zPos = sin(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);

            // pos (3), normal (3), texcoords (2)
            data.push_back(xPos);
            data.push_back(yPos);
            data.push_back(zPos);
            data.push_back(xPos); data.push_back(yPos); data.push_back(zPos);
            data.push_back(xSegment); data.push_back(ySegment);
        }
    }

    for (int y = 0; y < Y_SEGMENTS; ++y) {
        for (int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);

            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    GLsizei stride = (3+3+2) * sizeof(float);
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.indexCount = (GLsizei)indices.size();
    return mesh;
}

void Mesh::destroy() {
    if(ebo) glDeleteBuffers(1, &ebo);
    if(vbo) glDeleteBuffers(1, &vbo);
    if(vao) glDeleteVertexArrays(1, &vao);
}

Mesh createRing(float innerRadius, float outerRadius, int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve((segments+1)* (3+3+2) * 2);

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * (float)i / (float)segments;
        float cosA = cos(angle);
        float sinA = sin(angle);

        // Outer vertex
        vertices.push_back(cosA * outerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(sinA * outerRadius);
        // normal up
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        // texcoord
        vertices.push_back(1.0f); vertices.push_back((float)i / (float)segments);

        // Inner vertex
        vertices.push_back(cosA * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(sinA * innerRadius);
        // normal up
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        // texcoord
        vertices.push_back(0.0f); vertices.push_back((float)i / (float)segments);
    }

    // build indices
    for (int i = 0; i < segments; ++i) {
        unsigned int idx = i * 2;
        // triangle 1: outer_i, inner_i, inner_i+1
        indices.push_back(idx);
        indices.push_back(idx + 1);
        indices.push_back(idx + 3);
        // triangle 2: outer_i, inner_i+1, outer_i+1
        indices.push_back(idx);
        indices.push_back(idx + 3);
        indices.push_back(idx + 2);
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = (3 + 3 + 2) * sizeof(float);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    mesh.indexCount = (GLsizei)indices.size();
    return mesh;
}
