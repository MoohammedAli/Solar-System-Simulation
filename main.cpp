#include <glad/glad.h>   // Provides gl functions and pointers
#include <GLFW/glfw3.h>  // Window and context creation and input handling
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  //! Libraries starting with glm are math libraries and contain transformations & more
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <map>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"    //! This enables texturing (VERY SIMPLIFIED DEFINITION)
using namespace std;


// ---------- settings ----------
const unsigned int SCR_WIDTH = 1380;  // Window width
const unsigned int SCR_HEIGHT = 720;  // Window height

// ---------- camera ----------
glm::vec3 camPos   = glm::vec3(0.0f, 60.0f, 80.0f);  // How the camera is positioned right after opening program
glm::vec3 camFront = glm::vec3(0.0f, -0.3f, -1.0f);  // Dierction the camera is looking toward(This is normalized vector)
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);    // How the camera moves with mouse movement(it moves in Y-AXIS direction)

// Mouse Control State
float lastX = SCR_WIDTH/2.0f, lastY = SCR_HEIGHT/2.0f;  // This to get the position of the mouse on screen (lastX / lastY)
float yaw = -90.0f, pitch = -12.0f;   // Pitch is used to tilt the camera a little (- is down + is up)
bool firstMouse = true;
float fov = 90.0f;   // field of view (Sets how far the use pov is before moving the mouse)

bool orbitLines = true;  //? This toggles the otbit lines (turn off if you want to scale to avoid more calculations ;)

// timing (for frame timed movement)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// input
bool keys[1024];

// ---------- simple shader helper ----------
GLuint compileShader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info[1024]; glGetShaderInfoLog(id, 1024, nullptr, info);
        cerr << "Shader compile error: " << info << endl;
    }
    return id;
}

//! This makes the shaderprogram by combining both vertexShader and fragmentShader
GLuint createProgram(const char* vs, const char* fs) {
    GLuint program = glCreateProgram();
    GLuint a = compileShader(GL_VERTEX_SHADER, vs);
    GLuint b = compileShader(GL_FRAGMENT_SHADER, fs);
    glAttachShader(program, a); glAttachShader(program, b);
    glLinkProgram(program);
    int success; glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
      char info[1024];
      glGetProgramInfoLog(program, 1024, nullptr, info);
      cerr << "Link error: " << info << endl;
    }
    glDeleteShader(a); glDeleteShader(b);
    return program;
}

// ---------- sphere generator ----------
struct Mesh {
    GLuint vao, vbo, ebo;
    GLsizei indexCount;
};

Mesh createSphere(int X_SEGMENTS, int Y_SEGMENTS) {
    vector<float> data;
    vector<unsigned int> indices;
    for (int y = 0; y <= Y_SEGMENTS; ++y) {
        for (int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            // pos (3), normal (3), texcoords (2)
            data.push_back(xPos);
            data.push_back(yPos);
            data.push_back(zPos);
            data.push_back(xPos); data.push_back(yPos); data.push_back(zPos); // normal same as pos for unit sphere
            data.push_back(xSegment); data.push_back(ySegment);
        }
    }

    bool oddRow = false;
    for (int y = 0; y < Y_SEGMENTS; ++y) {
        for (int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y       * (X_SEGMENTS + 1) + x);
            indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);

            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    GLsizei stride = (3+3+2) * sizeof(float);
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.indexCount = (GLsizei)indices.size();
    return mesh;
}

// ---------- load texture ----------
GLuint loadTexture(const string &path) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if(!data) {
        cerr << "Failed to load texture: " << path << endl;
        return 0;
    }
    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

// ---------- load cubemap ----------
unsigned int loadCubemap(vector<string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    //! Cubemap textures should not be flipped vertically
    stbi_set_flip_vertically_on_load(false);
    for(unsigned int i=0;i<faces.size();i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
            stbi_image_free(data);
        }
        else
        {
            cout << "Cubemap failed to load: " << faces[i] << endl;
            stbi_image_free(data);
        }
    }
    // restore default vertical flip for other textures
    stbi_set_flip_vertically_on_load(true);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

// ---------- shader sources ----------
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTex;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform vec3 lightPos; // sun pos
uniform vec3 viewPos;
uniform bool isSun; // if true, render emissive

void main(){
    vec3 color = texture(texture1, TexCoords).rgb;

    if(isSun){
        // emissive bright look
        FragColor = vec4(color * 2.0, 1.0);
        return;
    }

    // ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * color;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * color;

    // specular (using white light)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(0.4) * spec;

    // attenuation (optional small)
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.002 * distance + 0.000001 * distance * distance);

    vec3 result = (ambient + attenuation*(diffuse + specular));
    FragColor = vec4(result, 1.0);
}
)glsl";

// skybox shaders
const char* skyboxVertexSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 view;
uniform mat4 projection;
void main(){
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos,1.0);
    gl_Position = pos.xyww;
}
)glsl";

const char* skyboxFragmentSrc = R"glsl(
#version 330 core
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
void main(){
    FragColor = texture(skybox,TexCoords);
}
)glsl";

// ---------- input callbacks ----------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        keys[key] = true;
        if (key == GLFW_KEY_O) orbitLines = !orbitLines;
    } else if(action == GLFW_RELEASE) {
        keys[key] = false;
    }
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if(firstMouse){ lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // reversed
    lastX = (float)xpos; lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;

    yaw += xoffset; pitch += yoffset;
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    fov -= (float)yoffset;
    if(fov < 20.0f) fov = 20.0f;
    if(fov > 80.0f) fov = 80.0f;
}

// ---------- camera movement processing ----------
void doMovement(float dt){
    float cameraSpeed = 20.0f * dt;
    if(keys[GLFW_KEY_W]) camPos += cameraSpeed * camFront;
    if(keys[GLFW_KEY_S]) camPos -= cameraSpeed * camFront;
    if(keys[GLFW_KEY_A]) camPos -= glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
    if(keys[GLFW_KEY_D]) camPos += glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
    if(keys[GLFW_KEY_Q]) camPos += cameraSpeed * camUp;
    if(keys[GLFW_KEY_E]) camPos -= cameraSpeed * camUp;
}

// ---------- planet data ----------
struct Planet {
    string name;
    float radius;        // visual size
    float distance;      // orbital radius
    float orbitPeriod;   // seconds to complete orbit (simulation time)
    float rotationPeriod; // seconds to rotate on axis
    GLuint texture;
    glm::vec3 color;
};

int main() {
    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Solar System", nullptr, nullptr);
    if(!window){ cerr << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){ cerr << "Failed to initialize GLAD\n"; return -1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // create sphere mesh
    Mesh sphere = createSphere(64, 64);

    // compile shader
    GLuint program = createProgram(vertexShaderSrc, fragmentShaderSrc);

    // load textures
    map<string,string> texFiles = {
        {"sun", "textures/sun.jpeg"},
        {"mercury","textures/mercury.jpeg"},
        {"venus","textures/venus.jpeg"},
        {"earth","textures/earth.jpeg"},
        {"mars","textures/mars.jpeg"},
        {"jupiter","textures/jupiter.jpeg"},
        {"saturn","textures/saturn.jpeg"},
        {"uranus","textures/uranus.jpeg"},
        {"neptune","textures/neptune.jpeg"}
    };
    map<string, GLuint> textures;
    for(auto &p : texFiles){
        textures[p.first] = loadTexture(p.second);
        if(textures[p.first] == 0) {
            cerr << "Warning: texture " << p.second << " failed to load. Using 1x1 placeholder.\n";
            unsigned char white[3] = {255,255,255};
            GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1,1,0,GL_RGB,GL_UNSIGNED_BYTE,white);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            textures[p.first] = t;
        }
    }

    // define planets (visual sizes & speeds chosen for demo, not to scale)
    vector<Planet> planets = {
        {"Sun",   6.0f,   0.0f,    0.0f,    25.0f, textures["sun"],    glm::vec3(1.0f,0.9f,0.6f)},
        {"Mercury", 0.6f, 10.0f,   10.0f,    10.0f, textures["mercury"], glm::vec3(0.6f)},
        {"Venus",   1.0f, 15.0f,   18.0f,   -20.0f, textures["venus"],   glm::vec3(1.0f,0.8f,0.6f)},
        {"Earth",   1.1f, 20.0f,   20.0f,    1.0f,  textures["earth"],   glm::vec3(0.4f,0.6f,1.0f)},
        {"Mars",    0.8f, 26.0f,   30.0f,    1.03f, textures["mars"],    glm::vec3(1.0f,0.5f,0.4f)},
        {"Jupiter", 2.4f, 36.0f,   60.0f,    0.4f,  textures["jupiter"], glm::vec3(1.0f,0.9f,0.7f)},
        {"Saturn",  2.0f, 48.0f,   80.0f,    0.45f, textures["saturn"],  glm::vec3(1.0f,0.9f,0.8f)},
        {"Uranus",  1.6f, 60.0f,  100.0f,    0.72f, textures["uranus"],  glm::vec3(0.6f,0.9f,1.0f)},
        {"Neptune", 1.6f, 72.0f,  130.0f,    0.67f, textures["neptune"], glm::vec3(0.4f,0.6f,1.0f)}
    };

    // pre-calc orbit circle vertices (for simple orbit visualization)
    vector<glm::vec3> circleVerts;
    const int CIRCLE_SEGMENTS = 180;
    for(int i=0;i<=CIRCLE_SEGMENTS;i++){
        float theta = 2.0f * M_PI * ((float)i / (float)CIRCLE_SEGMENTS);
        circleVerts.push_back(glm::vec3(cos(theta), 0.0f, sin(theta)));
    }
    GLuint orbitVAO, orbitVBO;
    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);
    glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVerts.size()*sizeof(glm::vec3), circleVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0); glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // uniforms
    glUseProgram(program);
    GLint uniModel = glGetUniformLocation(program, "model");
    GLint uniView = glGetUniformLocation(program, "view");
    GLint uniProj = glGetUniformLocation(program, "projection");
    GLint uniLightPos = glGetUniformLocation(program, "lightPos");
    GLint uniViewPos = glGetUniformLocation(program, "viewPos");
    GLint uniIsSun = glGetUniformLocation(program, "isSun");

    // create skybox shader program
    GLuint skyboxProgram = createProgram(skyboxVertexSrc, skyboxFragmentSrc);
    glUseProgram(skyboxProgram);
    GLint sbView = glGetUniformLocation(skyboxProgram, "view");
    GLint sbProj = glGetUniformLocation(skyboxProgram, "projection");
    // set sampler to texture unit 0
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);

    // ---------- skybox ----------
    float skyboxVertices[] = {
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      -1.0f,  1.0f, -1.0f,
      1.0f,  1.0f, -1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f, -1.0f,  1.0f
  };

    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays(1,&skyboxVAO); glGenBuffers(1,&skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER,skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(skyboxVertices),&skyboxVertices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

    vector<string> faces = {
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    // render loop
    float simulationTime = 0.0f;
    while(!glfwWindowShouldClose(window)){
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        simulationTime += deltaTime * 1.0f; // speed factor (1.0x)

        // input
        glfwPollEvents();
        doMovement(deltaTime);
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        // render
        glClearColor(0.02f,0.02f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(uniViewPos, 1, glm::value_ptr(camPos));

        // draw orbits
        if(orbitLines) {
            glUseProgram(program);
            glUniform1i(uniIsSun, GL_FALSE);
            glBindVertexArray(orbitVAO);
            for(size_t i=1;i<planets.size();++i){
                float r = planets[i].distance;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(r, 1.0f, r));
                glUniformMatrix4fv(uniModel,1,GL_FALSE,glm::value_ptr(model));
                // simple color by setting a white texture binding (we're using fragment shader textures,
                // but circle uses same shader; so to make it visible we'll bind sun texture but the color will be mostly ambient)
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures["sun"]);
                glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)circleVerts.size());
            }
            glBindVertexArray(0);
        }

        // draw planets
        glBindVertexArray(sphere.vao);
        for(size_t i=0;i<planets.size();++i){
            Planet &p = planets[i];

            // compute orbital transform
            glm::mat4 model = glm::mat4(1.0f);
            if(i != 0) {
                float orbitT = simulationTime / p.orbitPeriod; // fraction
                float angle = orbitT * 2.0f * (float)M_PI;
                float x = p.distance * cos(angle);
                float z = p.distance * sin(angle);
                model = glm::translate(model, glm::vec3(x, 0.0f, z));
            } else { // Sun at origin
                model = glm::translate(model, glm::vec3(0.0f));
            }

            // rotation on axis
            float rotAngle = 0.0f;
            if(p.rotationPeriod != 0.0f) {
                rotAngle = (simulationTime / p.rotationPeriod) * 360.0f;
                model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // scale
            model = glm::scale(model, glm::vec3(p.radius));

            glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

            // set light pos = sun position (origin)
            glm::vec3 sunPos(0.0f);
            glUniform3fv(uniLightPos, 1, glm::value_ptr(sunPos));

            // isSun flag toggles emissive shader behavior
            glUniform1i(uniIsSun, (i==0) ? GL_TRUE : GL_FALSE);

            // bind texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, p.texture);
            glUniform1i(glGetUniformLocation(program, "texture1"), 0);

            glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);



        // draw skybox (render last)
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxProgram);
        // remove translation from the view matrix
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(sbView, 1, GL_FALSE, glm::value_ptr(viewNoTrans));
        glUniformMatrix4fv(sbProj, 1, GL_FALSE, glm::value_ptr(proj));
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // swap buffers
        glfwSwapBuffers(window);
    }

    // cleanup
    glDeleteVertexArrays(1, &sphere.vao);
    glDeleteBuffers(1, &sphere.vbo);
    glDeleteBuffers(1, &sphere.ebo);
    // cleanup skybox resources
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteProgram(skyboxProgram);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}
