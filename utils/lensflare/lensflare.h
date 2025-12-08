#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "../../shader/shader.h"

struct FlareElement {
    float position;        // Position along sun-to-screen-center line (0=sun, 1=opposite)
    float size;           // Size of flare element
    glm::vec3 color;      // RGB tint
    float brightness;     // Opacity multiplier
    int textureIndex;     // Which texture to use (0-3)
};

class LensFlareSystem {
public:
    LensFlareSystem();
    ~LensFlareSystem();

    void init();
    void render(const glm::vec3 &sunWorldPos, const glm::mat4 &view,
                const glm::mat4 &proj, int screenWidth, int screenHeight);
    void cleanup();

    bool enabled = true;
    float globalIntensity = 1.0f;

private:
    std::unique_ptr<Shader> shader;
    GLuint quadVAO, quadVBO;
    std::vector<GLuint> flareTextures;  // Multiple flare textures
    std::vector<FlareElement> flareElements;

    void createQuad();
    void generateFlareTextures();
    glm::vec2 worldToScreen(const glm::vec3 &worldPos, const glm::mat4 &viewProj,
                            int screenWidth, int screenHeight, float &depth);
    float calculateOcclusion(const glm::vec3 &sunWorldPos, const glm::mat4 &view,
                            const glm::mat4 &proj);
};
