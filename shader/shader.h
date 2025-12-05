#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    GLuint ID;
    Shader();
    Shader(const char* vertexSrc, const char* fragmentSrc);
    // Create shader from vertex/fragment file paths
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    ~Shader();
    void use() const { glUseProgram(ID); }
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &v) const;
    void setInt(const std::string &name, int value) const;
};
