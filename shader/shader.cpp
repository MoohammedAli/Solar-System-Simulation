#include "shader.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>

static GLuint compileShaderInternal(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int success = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info[4096]; glGetShaderInfoLog(id, 4096, nullptr, info);
        // Print shader type and a short preview of the source to help debug
        const char* typeName = (type==GL_VERTEX_SHADER)?"VERTEX":"FRAGMENT";
        std::cerr << "Shader compile error (" << typeName << "): " << info << std::endl;
        std::string s(src?src:"(null)");
        std::string preview = s.substr(0, std::min<size_t>(s.size(), 512));
        std::cerr << "--- shader source preview ---\n" << preview << "\n--- end preview ---" << std::endl;
    }
    return id;
}

Shader::Shader() : ID(0) {}

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vs = compileShaderInternal(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShaderInternal(GL_FRAGMENT_SHADER, fragmentSrc);
    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);
    int success = 0; glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
        char info[1024]; glGetProgramInfoLog(ID, 1024, nullptr, info);
        std::cerr << "Link error: " << info << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
    std::ifstream vfile(vertexPath);
    std::ifstream ffile(fragmentPath);
    if(!vfile.is_open() || !ffile.is_open()) {
        std::cerr << "Failed to open shader files: " << vertexPath << " , " << fragmentPath << std::endl;
        ID = 0;
        return;
    }
    std::stringstream vss, fss;
    vss << vfile.rdbuf();
    fss << ffile.rdbuf();
    std::string vstr = vss.str();
    std::string fstr = fss.str();
    GLuint vs = compileShaderInternal(GL_VERTEX_SHADER, vstr.c_str());
    GLuint fs = compileShaderInternal(GL_FRAGMENT_SHADER, fstr.c_str());
    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);
    int success = 0; glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
        char info[1024]; glGetProgramInfoLog(ID, 1024, nullptr, info);
        std::cerr << "Link error: " << info << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    if(ID) glDeleteProgram(ID);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if(loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string &name, const glm::vec3 &v) const {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if(loc != -1) glUniform3fv(loc, 1, &v[0]);
}

void Shader::setInt(const std::string &name, int value) const {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if(loc != -1) glUniform1i(loc, value);
}
