#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>

GLuint loadTexture(const std::string &path);
unsigned int loadCubemap(const std::vector<std::string> &faces);
