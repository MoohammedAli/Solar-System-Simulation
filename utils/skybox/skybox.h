#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../../shader/shader.h"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);
    ~Skybox();
    void render(const glm::mat4& view, const glm::mat4& proj);
private:
    GLuint VAO, VBO;
    unsigned int cubemapTex;
    Shader shader;
};
