#pragma once
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "../mesh/mesh.h"
#include "../../shader/shader.h"
#include "../dust/dust.h"
#include "../asteroids/asteroids.h"
#include "../lensflare/lensflare.h"
#include <memory>
using namespace std;

struct Planet {
    string name;
    float radius;
    float distance;
    float orbitPeriod;
    float rotationPeriod;
    GLuint texture;
    glm::vec3 color;
    bool hasAtmosphere;
    glm::vec3 atmosphereColor;
    float atmosphereIntensity;
};

struct Moon {
    float distance;
    float orbitPeriod;
    float radius;
};

class Scene {
private:
    vector<Planet> planets;
    map<string, string> texFiles;
    map<string, GLuint> textures;
    GLuint moonTexture;
    vector<glm::vec3> circleVerts;
    GLuint orbitVAO, orbitVBO;
    void setupOrbits();
    std::unique_ptr<DustSystem> dustSystem;
    std::unique_ptr<AsteroidSystem> asteroidSystem;
    std::unique_ptr<Shader> atmosphereShader;
    std::unique_ptr<LensFlareSystem> lensFlareSystem;
    Mesh saturnRing;
    GLuint saturnRingTexture;
    std::vector<Moon> jupiterMoons;

public:
    bool showAsteroids = true;
    bool showDust = true;
    bool showRings = true;
    bool showAtmospheres = true;
    bool showLensFlare = true;

    Scene();
    void init();
    void render(Shader &planetShader, const glm::mat4 &view, const glm::mat4 &proj,
                const glm::vec3 &camPos, const glm::vec3 &camFront, const glm::vec3 &camUp,
                Mesh &sphere, float simulationTime, float deltaTime, int screenWidth, int screenHeight);
    void cleanup();

    glm::vec3 getPlanetPosition(int planetIndex, float simulationTime);
    void renderAtmospheres(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos, Mesh &sphere, float simulationTime);
};
