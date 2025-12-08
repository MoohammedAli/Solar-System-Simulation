#include "lensflare.h"
#include <iostream>
#include <cmath>

LensFlareSystem::LensFlareSystem() : quadVAO(0), quadVBO(0) {}

LensFlareSystem::~LensFlareSystem() {
    cleanup();
}

void LensFlareSystem::init() {
    // Create shader
    shader = std::make_unique<Shader>(
        std::string("shader/lensflare.vert"),
        std::string("shader/lensflare.frag")
    );

    createQuad();
    generateFlareTextures();

    // Define flare elements
    // Position: 0 = at sun, 1 = opposite side of screen, negative = beyond sun
    flareElements = {
        // Main glow at sun
        {0.0f,  0.8f,  glm::vec3(1.0f, 0.9f, 0.7f),  1.0f,  0},
        {0.0f,  0.5f,  glm::vec3(1.0f, 1.0f, 1.0f),  0.6f,  1},

        // Streaks and halos
        {0.1f,  0.15f, glm::vec3(1.0f, 0.8f, 0.5f),  0.5f,  2},
        {0.3f,  0.12f, glm::vec3(0.8f, 0.9f, 1.0f),  0.4f,  3},
        {0.5f,  0.18f, glm::vec3(1.0f, 0.7f, 0.4f),  0.4f,  2},
        {0.7f,  0.10f, glm::vec3(0.9f, 0.8f, 1.0f),  0.3f,  3},
        {0.9f,  0.14f, glm::vec3(1.0f, 0.9f, 0.6f),  0.35f, 2},

        // Ghost flares (opposite side)
        {1.2f,  0.20f, glm::vec3(0.7f, 0.9f, 1.0f),  0.3f,  1},
        {1.5f,  0.16f, glm::vec3(1.0f, 0.8f, 0.7f),  0.25f, 3},
        {1.8f,  0.12f, glm::vec3(0.8f, 1.0f, 0.9f),  0.2f,  2},
    };

    std::cout << "Lens flare system initialized with " << flareElements.size()
              << " elements" << std::endl;
}

void LensFlareSystem::createQuad() {
    // Simple quad: two triangles
    float quadVertices[] = {
        // Positions    // TexCoords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void LensFlareSystem::generateFlareTextures() {
    const int TEX_SIZE = 128;

    // Create 4 different flare textures procedurally
    for (int texIdx = 0; texIdx < 4; ++texIdx) {
        std::vector<unsigned char> texData(TEX_SIZE * TEX_SIZE * 4);

        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                float dx = (x - TEX_SIZE / 2.0f) / (TEX_SIZE / 2.0f);
                float dy = (y - TEX_SIZE / 2.0f) / (TEX_SIZE / 2.0f);
                float dist = std::sqrt(dx * dx + dy * dy);

                int idx = (y * TEX_SIZE + x) * 4;
                float intensity = 0.0f;

                switch(texIdx) {
                    case 0: // Soft circular glow
                        intensity = std::max(0.0f, 1.0f - dist);
                        intensity = std::pow(intensity, 2.0f);
                        break;

                    case 1: // Ring/halo
                        intensity = std::exp(-std::pow((dist - 0.7f) * 5.0f, 2.0f));
                        break;

                    case 2: // Sharp circle
                        intensity = (dist < 0.8f) ? (1.0f - dist / 0.8f) : 0.0f;
                        intensity = std::pow(intensity, 3.0f);
                        break;

                    case 3: // Hexagonal (lens aperture shape)
                        {
                            float angle = std::atan2(dy, dx);
                            float hexDist = dist * (1.0f + 0.2f * std::cos(angle * 6.0f));
                            intensity = std::max(0.0f, 1.0f - hexDist);
                            intensity = std::pow(intensity, 2.5f);
                        }
                        break;
                }

                // Add subtle noise for realism
                float noise = (std::sin(x * 0.5f) * std::cos(y * 0.3f) + 1.0f) * 0.5f;
                intensity *= (0.9f + noise * 0.1f);

                unsigned char value = static_cast<unsigned char>(intensity * 255.0f);
                texData[idx + 0] = value;  // R
                texData[idx + 1] = value;  // G
                texData[idx + 2] = value;  // B
                texData[idx + 3] = value;  // A
            }
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, texData.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        flareTextures.push_back(texture);
    }

    std::cout << "Generated " << flareTextures.size() << " lens flare textures" << std::endl;
}

glm::vec2 LensFlareSystem::worldToScreen(const glm::vec3 &worldPos, const glm::mat4 &viewProj,
                                         int screenWidth, int screenHeight, float &depth) {
    // Transform to clip space
    glm::vec4 clipSpace = viewProj * glm::vec4(worldPos, 1.0f);

    // Perspective divide
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
    depth = ndc.z;

    // Convert to screen coordinates [-1, 1] (used for rendering in NDC)
    return glm::vec2(ndc.x, ndc.y);
}

float LensFlareSystem::calculateOcclusion(const glm::vec3 &sunWorldPos,
                                         const glm::mat4 &view, const glm::mat4 &proj) {
    // Simple occlusion based on sun visibility
    // In a full implementation, you'd read depth buffer to check if planets block the sun

    glm::mat4 viewProj = proj * view;
    glm::vec4 clipSpace = viewProj * glm::vec4(sunWorldPos, 1.0f);

    // Check if sun is behind camera
    if (clipSpace.w < 0.0f) return 0.0f;

    // Perspective divide
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

    // Check if sun is outside view frustum
    if (ndc.x < -1.0f || ndc.x > 1.0f ||
        ndc.y < -1.0f || ndc.y > 1.0f ||
        ndc.z < -1.0f || ndc.z > 1.0f) {
        return 0.0f;
    }

    // Fade at screen edges
    float edgeFade = 1.0f;
    float edgeX = std::abs(ndc.x);
    float edgeY = std::abs(ndc.y);

    if (edgeX > 0.8f) edgeFade *= (1.0f - (edgeX - 0.8f) / 0.2f);
    if (edgeY > 0.8f) edgeFade *= (1.0f - (edgeY - 0.8f) / 0.2f);

    return edgeFade;
}

void LensFlareSystem::render(const glm::vec3 &sunWorldPos, const glm::mat4 &view,
                             const glm::mat4 &proj, int screenWidth, int screenHeight) {
    if (!enabled || !shader) return;

    // Calculate occlusion
    float occlusion = calculateOcclusion(sunWorldPos, view, proj);
    if (occlusion < 0.01f) return;  // Sun not visible

    // Get sun screen position
    float sunDepth;
    glm::mat4 viewProj = proj * view;
    glm::vec2 sunScreenPos = worldToScreen(sunWorldPos, viewProj, screenWidth, screenHeight, sunDepth);

    // Screen center is at (0, 0) in NDC
    glm::vec2 screenCenter(0.0f, 0.0f);

    // Vector from sun to screen center
    glm::vec2 sunToCenter = screenCenter - sunScreenPos;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for bright flares
    glDisable(GL_DEPTH_TEST);  // Always draw on top

    shader->use();
    glBindVertexArray(quadVAO);

    // Render each flare element
    for (const auto &element : flareElements) {
        // Calculate flare position along sun-to-center line
        glm::vec2 flarePos = sunScreenPos + sunToCenter * element.position;

        // Skip if too far off screen
        if (std::abs(flarePos.x) > 2.0f || std::abs(flarePos.y) > 2.0f) continue;

        // Calculate opacity (fade with distance from sun and occlusion)
        float distFromSun = glm::length(sunToCenter) * std::abs(element.position);
        float distanceFade = 1.0f / (1.0f + distFromSun * 2.0f);
        float opacity = element.brightness * occlusion * distanceFade * globalIntensity;

        if (opacity < 0.01f) continue;

        // Set uniforms
        glUniform2f(glGetUniformLocation(shader->ID, "flarePosition"), flarePos.x, flarePos.y);
        glUniform1f(glGetUniformLocation(shader->ID, "flareSize"), element.size);
        glUniform3f(glGetUniformLocation(shader->ID, "flareColor"),
                   element.color.r, element.color.g, element.color.b);
        glUniform1f(glGetUniformLocation(shader->ID, "flareOpacity"), opacity);

        // Bind appropriate texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flareTextures[element.textureIndex]);
        glUniform1i(glGetUniformLocation(shader->ID, "flareTexture"), 0);

        // Draw quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);

    // Restore state
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void LensFlareSystem::cleanup() {
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);

    for (GLuint tex : flareTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }
    flareTextures.clear();

    shader.reset();
}
