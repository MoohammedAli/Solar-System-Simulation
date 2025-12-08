#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
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

// Time control
float timeScale = 1.0f;
bool showUI = true;

// Camera focus
int focusedPlanet = -1;
float focusDistance = 15.0f;
bool smoothCamera = false;
glm::vec3 targetCamPos = camPos;

// forward scene pointer for key toggles
static class Scene* g_scene = nullptr;

// Planet names for UI
const char* planetNames[] = {"Sun", "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"};

// input callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
  glViewport(0,0,width,height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
  if(action==GLFW_PRESS) {
    keys[key]=true;
    if(g_scene) {
      if(key == GLFW_KEY_B) { g_scene->showAsteroids = !g_scene->showAsteroids; std::cout<<"Asteroids: "<<(g_scene->showAsteroids?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_V) { g_scene->showDust = !g_scene->showDust; std::cout<<"Dust: "<<(g_scene->showDust?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_R) { g_scene->showRings = !g_scene->showRings; std::cout<<"Rings: "<<(g_scene->showRings?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_G) { g_scene->showAtmospheres = !g_scene->showAtmospheres; std::cout<<"Atmospheres: "<<(g_scene->showAtmospheres?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_L) { g_scene->showLensFlare = !g_scene->showLensFlare; std::cout<<"Lens Flare: "<<(g_scene->showLensFlare?"ON":"OFF")<<"\n"; }
      if(key == GLFW_KEY_H) { showUI = !showUI; }

      // Time control
      if(key == GLFW_KEY_SPACE) { timeScale = (timeScale == 0.0f) ? 1.0f : 0.0f; std::cout<<"Time: "<<(timeScale==0.0f?"PAUSED":"RUNNING")<<"\n"; }
      if(key == GLFW_KEY_COMMA) { timeScale = max(0.0f, timeScale - 0.5f); std::cout<<"Time scale: "<<timeScale<<"x\n"; }
      if(key == GLFW_KEY_PERIOD) { timeScale = min(10.0f, timeScale + 0.5f); std::cout<<"Time scale: "<<timeScale<<"x\n"; }

      // Planet focus (1-9 keys)
      if(key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        int planetIdx = key - GLFW_KEY_1;
        if(planetIdx == focusedPlanet) {
          focusedPlanet = -1;
          smoothCamera = false;
          std::cout<<"Camera: Free mode\n";
        } else {
          focusedPlanet = planetIdx;
          smoothCamera = true;
          std::cout<<"Focus: "<<planetNames[planetIdx]<<"\n";
        }
      }
      if(key == GLFW_KEY_0) { focusedPlanet = -1; smoothCamera = false; std::cout<<"Camera: Free mode\n"; }
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

    // Disable mouse control when focusing on planet
    if(focusedPlanet >= 0) return;

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
  if(focusedPlanet >= 0) {
    focusDistance = glm::clamp(focusDistance - (float)yoffset * 2.0f, 5.0f, 100.0f);
  } else {
    fov -= (float)yoffset;
    if(fov<20.0f) fov=20.0f;
    if(fov>80.0f) fov=80.0f;
  }
}

void doMovement(float dt){
  if(focusedPlanet >= 0) return; // Disable manual movement when focused

  float cameraSpeed = 20.0f * dt;
  if(keys[GLFW_KEY_W]) camPos += cameraSpeed * camFront;
  if(keys[GLFW_KEY_S]) camPos -= cameraSpeed * camFront;
  if(keys[GLFW_KEY_A]) camPos -= glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
  if(keys[GLFW_KEY_D]) camPos += glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
  if(keys[GLFW_KEY_Q]) camPos += cameraSpeed * camUp;
  if(keys[GLFW_KEY_E]) camPos -= cameraSpeed * camUp;
}

void updateFocusCamera(Scene& scene, float simulationTime, float dt) {
  if(focusedPlanet < 0 || focusedPlanet >= 9) return;

  // Get planet position
  glm::vec3 planetPos = scene.getPlanetPosition(focusedPlanet, simulationTime);

  // Calculate target camera position
  targetCamPos = planetPos + glm::vec3(focusDistance * 0.5f, focusDistance * 0.3f, focusDistance);

  // Smooth interpolation
  if(smoothCamera) {
    float smoothSpeed = 2.0f * dt;
    camPos = glm::mix(camPos, targetCamPos, smoothSpeed);
  } else {
    camPos = targetCamPos;
  }

  // Look at planet
  camFront = glm::normalize(planetPos - camPos);
}

void displayUI(GLFWwindow* window, Scene& scene, float simulationTime, float fps) {
  if(!showUI) return;

  std::stringstream ss;
  ss << "Solar System | FPS: " << (int)fps
     << " | Time: " << std::fixed << std::setprecision(1) << timeScale << "x";

  if(timeScale == 0.0f) ss << " [PAUSED]";
  if(focusedPlanet >= 0) ss << " | Focus: " << planetNames[focusedPlanet];

  ss << " | [H]elp [Space]Pause [,.]Speed [1-9]Focus [B]elts [V]Dust [R]ings [G]low [L]Flare";

  glfwSetWindowTitle(window, ss.str().c_str());
}

int main(){
    if(!glfwInit()){
      cerr<<"GLFW init failed"<<endl;
      return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Simulator", NULL, NULL);
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // create sphere mesh
    Mesh sphere = createSphere(64,64);

    // compile shader from files
    Shader planetShader(std::string("shader/planet.vert"), std::string("shader/planet.frag"));

    // create and initialize scene
    Scene scene;
    scene.init();
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
    int frameCount = 0;
    float fpsTimer = 0.0f;
    float currentFPS = 0.0f;

    std::cout << "\n=== SOLAR SYSTEM CONTROLS ===\n";
    std::cout << "WASD + Q/E: Move camera\n";
    std::cout << "Mouse: Look around\n";
    std::cout << "1-9: Focus on planet\n";
    std::cout << "0: Free camera\n";
    std::cout << "Space: Pause/Resume\n";
    std::cout << ", / . : Decrease/Increase time speed\n";
    std::cout << "B: Toggle asteroids\n";
    std::cout << "V: Toggle dust\n";
    std::cout << "R: Toggle Saturn rings\n";
    std::cout << "G: Toggle atmospheric glow\n";
    std::cout << "L: Toggle lens flare\n";
    std::cout << "H: Toggle UI\n";
    std::cout << "ESC: Exit\n";
    std::cout << "============================\n\n";

    while(!glfwWindowShouldClose(window)){
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        simulationTime += deltaTime * timeScale;

        // FPS calculation
        frameCount++;
        fpsTimer += deltaTime;
        if(fpsTimer >= 1.0f) {
          currentFPS = frameCount / fpsTimer;
          frameCount = 0;
          fpsTimer = 0.0f;
        }

        glfwPollEvents();
        doMovement(deltaTime);
        updateFocusCamera(scene, simulationTime, deltaTime);

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        // Render scene with screen dimensions for lens flare
        scene.render(planetShader, view, proj, camPos, camFront, camUp, sphere,
                    simulationTime, deltaTime, SCR_WIDTH, SCR_HEIGHT);

        skybox.render(view, proj);

        displayUI(window, scene, simulationTime, currentFPS);

        glfwSwapBuffers(window);
    }

    scene.cleanup();
    sphere.destroy();
    glfwTerminate();
    return 0;
}
