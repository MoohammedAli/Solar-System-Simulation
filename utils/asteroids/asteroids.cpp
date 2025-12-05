#include "asteroids.h"
#include "../texture/texture.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

AsteroidSystem::AsteroidSystem() : asteroidTexture(0) {}
AsteroidSystem::~AsteroidSystem() { cleanup(); }

void AsteroidSystem::init() {
    asteroids.clear(); asteroids.reserve(ASTEROID_COUNT);
    srand(static_cast<unsigned>(time(nullptr)));
    for (int i = 0; i < ASTEROID_COUNT; ++i) {
        Asteroid ast;
        ast.radius = 0.04f + static_cast<float>(rand()) / RAND_MAX * 0.15f;
        ast.distance = 30.0f + static_cast<float>(rand()) / RAND_MAX * (33.0f - 30.0f);
        ast.inclination = -0.05f + static_cast<float>(rand()) / RAND_MAX * 0.1f;
        ast.orbitalPhase = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159265358979323846f;
        ast.rotationSpeed = 15.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f;
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159265358979323846f;
        float phi = static_cast<float>(rand()) / RAND_MAX * 3.14159265358979323846f;
        ast.rotationAxis = glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
        asteroids.push_back(ast);
    }

    asteroidTexture = loadTexture("utils/textures/asteroid.jpg");
    if (asteroidTexture == 0) {
        const int TEX_SIZE = 128;
        std::vector<unsigned char> asteroidTex(TEX_SIZE * TEX_SIZE * 3);
        for (int y = 0; y < TEX_SIZE; ++y) for (int x = 0; x < TEX_SIZE; ++x) {
            int idx = (y * TEX_SIZE + x) * 3;
            unsigned char gray = 150;
            asteroidTex[idx+0] = gray; asteroidTex[idx+1] = gray; asteroidTex[idx+2] = gray;
        }
        glGenTextures(1, &asteroidTexture);
        glBindTexture(GL_TEXTURE_2D, asteroidTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, asteroidTex.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void AsteroidSystem::render(float simulationTime, Mesh &sphere, Shader &planetShader) {
    // use the provided planet shader for consistent lighting
    planetShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asteroidTexture);
    planetShader.setInt("texture1", 0);
    planetShader.setInt("isSun", 0);

    float asteroidOrbitPeriod = 70.0f;
    float asteroidOrbitT = simulationTime / asteroidOrbitPeriod;

    glBindVertexArray(sphere.vao);
    for (const auto& ast : asteroids) {
        float orbitAngle = ast.orbitalPhase + asteroidOrbitT * 2.0f * 3.14159265358979323846f;
        float x = ast.distance * cos(orbitAngle);
        float z = ast.distance * sin(orbitAngle);
        float y = sin(ast.inclination) * 1.5f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, z));
        float rotationAngle = simulationTime * ast.rotationSpeed;
        model = glm::rotate(model, glm::radians(rotationAngle), ast.rotationAxis);
        model = glm::scale(model, glm::vec3(ast.radius));
        planetShader.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void AsteroidSystem::cleanup() {
    if (asteroidTexture) glDeleteTextures(1, &asteroidTexture);
}
