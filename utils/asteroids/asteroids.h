#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "../mesh/mesh.h"
#include "../../shader/shader.h"

struct Asteroid {
    float radius;
    float distance;
    float inclination;
    float orbitalPhase;
    float rotationSpeed;
    glm::vec3 rotationAxis;
};

class AsteroidSystem {
public:
    AsteroidSystem();
    ~AsteroidSystem();
    void init();
    void render(float simulationTime, Mesh &sphere, Shader &planetShader);
    void cleanup();
private:
    std::vector<Asteroid> asteroids;
    GLuint asteroidTexture;
    const int ASTEROID_COUNT = 2000;
};
