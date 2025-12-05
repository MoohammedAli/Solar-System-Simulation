#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "../mesh/mesh.h"
#include "../../shader/shader.h"

struct SpaceDustParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float life;
    float rotation;
    float rotationSpeed;
};

class DustSystem {
public:
    DustSystem();
    ~DustSystem();
    void init();
    void update(float deltaTime);
    void render(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camFront, const glm::vec3 &camUp);
    void cleanup();
private:
    std::vector<SpaceDustParticle> dust;
    Mesh quad;
    GLuint dustTexture;
    std::unique_ptr<Shader> shader;

    // uniform locations (queried after shader program creation)
    GLint uniModel, uniView, uniProj, uniCameraRight, uniCameraUp, uniSize, uniLightColor;
};
