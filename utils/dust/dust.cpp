#include "dust.h"
#include "../texture/texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <GLFW/glfw3.h>

using namespace std;

// Parameters (copied from additions.cpp)
static const int DUST_PARTICLES = 5000;
static const float DUST_MIN_DISTANCE = 50.0f;
static const float DUST_MAX_DISTANCE = 300.0f;
static const float DUST_SIZE = 0.03f;
static const float DUST_VELOCITY = 0.5f;
static const float DUST_ROTATION_SPEED = 10.0f;

extern float getTimeSeconds(); // optional hook; we will use glfwGetTime directly in code when needed

DustSystem::DustSystem() : dustTexture(0), uniModel(-1), uniView(-1), uniProj(-1), uniCameraRight(-1), uniCameraUp(-1), uniSize(-1), uniLightColor(-1) {}

DustSystem::~DustSystem() { cleanup(); }

static Mesh createDustQuad() {
    float vertices[] = {
        // positions    // texCoords
        -0.5f,  0.5f,   0.0f, 1.0f,
        -0.5f, -0.5f,   0.0f, 0.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
         0.5f,  0.5f,   1.0f, 1.0f
    };

    unsigned int indices[] = { 0,1,2, 0,2,3 };

    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // pos (2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // tex (2)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    mesh.indexCount = 6;
    return mesh;
}

void DustSystem::init() {
    srand(static_cast<unsigned>(time(nullptr)));
    dust.clear(); dust.reserve(DUST_PARTICLES);

    for (int i = 0; i < DUST_PARTICLES; ++i) {
        SpaceDustParticle p;
        float radius = DUST_MIN_DISTANCE + static_cast<float>(rand()) / RAND_MAX * (DUST_MAX_DISTANCE - DUST_MIN_DISTANCE);
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159265358979323846f;
        float phi = acos(2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);
        p.position.x = radius * sin(phi) * cos(theta);
        p.position.y = radius * sin(phi) * sin(theta) * 0.3f;
        p.position.z = radius * cos(phi);
        glm::vec3 tangent = glm::normalize(glm::cross(p.position, glm::vec3(0.0f,1.0f,0.0f)));
        p.velocity = tangent * (DUST_VELOCITY + static_cast<float>(rand()) / RAND_MAX * 0.3f);
        p.size = DUST_SIZE * (0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f);
        p.life = 1.0f;
        p.rotation = static_cast<float>(rand()) / RAND_MAX * 360.0f;
        p.rotationSpeed = DUST_ROTATION_SPEED * (0.5f + static_cast<float>(rand()) / RAND_MAX);
        dust.push_back(p);
    }

    quad = createDustQuad();

    // load dust texture (fallback to procedural)
    dustTexture = loadTexture("utils/textures/dust_particle.png");
    if (dustTexture == 0) {
        const int TEX_SIZE = 64;
        std::vector<unsigned char> tex(TEX_SIZE * TEX_SIZE * 4);
        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                float dx = (x - TEX_SIZE / 2.0f) / (TEX_SIZE / 2.0f);
                float dy = (y - TEX_SIZE / 2.0f) / (TEX_SIZE / 2.0f);
                float dist = sqrt(dx * dx + dy * dy);
                int idx = (y * TEX_SIZE + x) * 4;
                if (dist > 1.0f) { tex[idx+0]=tex[idx+1]=tex[idx+2]=tex[idx+3]=0; }
                else {
                    float intensity = pow(1.0f - dist, 2.0f);
                    unsigned char value = (unsigned char)(200 * intensity + 55);
                    unsigned char r = value;
                    unsigned char g = (unsigned char)(value * 0.95f);
                    unsigned char b = (unsigned char)(value * 0.9f);
                    unsigned char a = (unsigned char)(255 * intensity);
                    tex[idx+0]=r; tex[idx+1]=g; tex[idx+2]=b; tex[idx+3]=a;
                }
            }
        }
        glGenTextures(1, &dustTexture);
        glBindTexture(GL_TEXTURE_2D, dustTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // compile dust shader using file-based Shader
    shader = std::make_unique<Shader>(std::string("shader/dust.vert"), std::string("shader/dust.frag"));
    shader->use();
    uniModel = glGetUniformLocation(shader->ID, "model");
    uniView = glGetUniformLocation(shader->ID, "view");
    uniProj = glGetUniformLocation(shader->ID, "projection");
    uniCameraRight = glGetUniformLocation(shader->ID, "cameraRight");
    uniCameraUp = glGetUniformLocation(shader->ID, "cameraUp");
    uniSize = glGetUniformLocation(shader->ID, "size");
    uniLightColor = glGetUniformLocation(shader->ID, "lightColor");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void DustSystem::update(float deltaTime) {
    for (auto &p : dust) {
        p.position += p.velocity * deltaTime;
        p.rotation += p.rotationSpeed * deltaTime;
        if (p.rotation > 360.0f) p.rotation -= 360.0f;
        p.life = 0.8f + 0.2f * sin(glfwGetTime() * 2.0f + p.position.x);
    }
}

void DustSystem::render(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camFront, const glm::vec3 &camUp) {
    if (!shader) return;
    shader->use();
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    // camera basis
    glm::vec3 cameraRight = glm::normalize(glm::cross(camFront, camUp));
    glm::vec3 cameraUpV = glm::normalize(glm::cross(cameraRight, camFront));
    glUniform3fv(uniCameraRight, 1, glm::value_ptr(cameraRight));
    glUniform3fv(uniCameraUp, 1, glm::value_ptr(cameraUpV));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dustTexture);

    glBindVertexArray(quad.vao);
    for (const auto &p : dust) {
        if (p.life <= 0.0f) continue;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.position);
        model = glm::rotate(model, glm::radians(p.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(uniSize, p.size * p.life);
        glDrawElements(GL_TRIANGLES, quad.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void DustSystem::cleanup() {
    if (quad.vbo) glDeleteBuffers(1, &quad.vbo);
    if (quad.ebo) glDeleteBuffers(1, &quad.ebo);
    if (quad.vao) glDeleteVertexArrays(1, &quad.vao);
    if (dustTexture) glDeleteTextures(1, &dustTexture);
    // Shader destructor will delete program
    shader.reset();
}
