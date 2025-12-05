#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include "shader/shader.h"
#include "utils/mesh/mesh.h"
#include "utils/texture/texture.h"
#include "utils/skybox/skybox.h"
#include "utils/scene/scene.h"
using namespace std;

// ---------- settings ----------
const unsigned int SCR_WIDTH = 1380;
const unsigned int SCR_HEIGHT = 720;

// camera
glm::vec3 camPos   = glm::vec3(0.0f, 60.0f, 80.0f);
glm::vec3 camFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH/2.0f, lastY = SCR_HEIGHT/2.0f;
float yaw = -90.0f, pitch = -12.0f;
bool firstMouse = true;
float fov = 60.0f;

bool keys[1024];

float deltaTime = 0.0f, lastFrame = 0.0f;

// forward scene pointer for key toggles
static class Scene* g_scene = nullptr;

// input callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
  glViewport(0,0,width,height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
  if(action==GLFW_PRESS) {
    keys[key]=true;
    if(g_scene) {
      if(key == GLFW_KEY_B) { g_scene->showAsteroids = !g_scene->showAsteroids; std::cout<<"Asteroids: "<<(g_scene->showAsteroids?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_D) { g_scene->showDust = !g_scene->showDust; std::cout<<"Dust: "<<(g_scene->showDust?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_R) { g_scene->showRings = !g_scene->showRings; std::cout<<"Rings: "<<(g_scene->showRings?"ON":"OFF")<<"\n"; }
    }
  }
  else if(action==GLFW_RELEASE) keys[key]=false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if(firstMouse){
      lastX=(float)xpos;
      lastY=(float)ypos;
      firstMouse=false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX=(float)xpos; lastY=(float)ypos;
    float sensitivity = 0.1f; xoffset*=sensitivity; yoffset*=sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if(pitch>89.0f) pitch=89.0f;
    if(pitch<-89.0f) pitch=-89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
  fov -= (float)yoffset;
  if(fov<20.0f) fov=20.0f;
  if(fov>80.0f) fov=80.0f;
}

void doMovement(float dt){
  float cameraSpeed = 20.0f * dt;
  if(keys[GLFW_KEY_W]) camPos += cameraSpeed * camFront;
  if(keys[GLFW_KEY_S]) camPos -= cameraSpeed * camFront;
  if(keys[GLFW_KEY_A]) camPos -= glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
  if(keys[GLFW_KEY_D]) camPos += glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
  if(keys[GLFW_KEY_Q]) camPos += cameraSpeed * camUp; if(keys[GLFW_KEY_E]) camPos -= cameraSpeed * camUp; }

// planet shaders are loaded from files: shader/planet.vert and shader/planet.frag

int main(){
    if(!glfwInit()){
      cerr<<"GLFW init failed"<<endl;
      return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CGProject", NULL, NULL);
    if(!window){
      cerr<<"Window creation failed"<<endl;
      glfwTerminate();
      return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
      cerr<<"Failed to init GLAD"<<endl;
      return -1;
    }

    glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);

    // create sphere mesh
    Mesh sphere = createSphere(64,64);

  // compile shader from files
  Shader planetShader(std::string("shader/planet.vert"), std::string("shader/planet.frag"));

    // create and initialize scene
    Scene scene; scene.init();
    g_scene = &scene;

    // skybox
    vector<string> faces = {
      "utils/skybox/right.jpg",
      "utils/skybox/left.jpg",
      "utils/skybox/top.jpg",
      "utils/skybox/bottom.jpg",
      "utils/skybox/front.jpg",
      "utils/skybox/back.jpg"
    };
    Skybox skybox(faces);

    float simulationTime = 0.0f;
    while(!glfwWindowShouldClose(window)){
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame; lastFrame = currentFrame;
        simulationTime += deltaTime;

        glfwPollEvents(); doMovement(deltaTime);
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        glClearColor(0.02f,0.02f,0.05f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        scene.render(planetShader, view, proj, camPos, camFront, camUp, sphere, simulationTime, deltaTime);
        skybox.render(view, proj);

        glfwSwapBuffers(window);
    }

    scene.cleanup(); sphere.destroy();
    glfwTerminate();
    return 0;
}
