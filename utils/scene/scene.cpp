#include <glad/glad.h>
#include "scene.h"
#include "../texture/texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include "../dust/dust.h"
#include "../asteroids/asteroids.h"
#include <memory>
using namespace std;

Scene::Scene() : orbitVAO(0), orbitVBO(0), moonTexture(0) {
    texFiles = {
        {"sun", "utils/textures/sun.jpeg"},
        {"mercury","utils/textures/mercury.jpeg"},
        {"venus","utils/textures/venus.jpeg"},
        {"earth","utils/textures/earth.jpeg"},
        {"mars","utils/textures/mars.jpeg"},
        {"jupiter","utils/textures/jupiter.jpeg"},
        {"saturn","utils/textures/saturn.jpeg"},
        {"uranus","utils/textures/uranus.jpeg"},
        {"neptune","utils/textures/neptune.jpeg"}
    };

    planets = {
        {"Sun",     6.0f,  0.0f,   0.0f,   25.0f, 0, glm::vec3(1.0f,0.9f,0.6f), false, glm::vec3(1.0f, 0.8f, 0.3f), 0.0f},
        {"Mercury", 0.6f, 10.0f,  10.0f,   10.0f, 0, glm::vec3(0.6f), false, glm::vec3(0.0f), 0.0f},
        {"Venus",   1.0f, 15.0f,  18.0f,  -20.0f, 0, glm::vec3(1.0f,0.8f,0.6f), true, glm::vec3(1.0f, 0.7f, 0.3f), 1.2f},
        {"Earth",   1.1f, 20.0f,  20.0f,    1.0f, 0, glm::vec3(0.4f,0.6f,1.0f), true, glm::vec3(0.3f, 0.5f, 1.0f), 1.5f},
        {"Mars",    0.8f, 26.0f,  30.0f,    1.03f, 0, glm::vec3(1.0f,0.5f,0.4f), true, glm::vec3(1.0f, 0.5f, 0.3f), 0.4f},
        {"Jupiter", 2.4f, 36.0f,  60.0f,    0.4f, 0, glm::vec3(1.0f,0.9f,0.7f), true, glm::vec3(0.9f, 0.8f, 0.6f), 0.8f},
        {"Saturn",  2.0f, 48.0f,  80.0f,    0.45f, 0, glm::vec3(1.0f,0.9f,0.8f), true, glm::vec3(1.0f, 0.9f, 0.7f), 0.6f},
        {"Uranus",  1.6f, 60.0f, 100.0f,    0.72f, 0, glm::vec3(0.6f,0.9f,1.0f), true, glm::vec3(0.5f, 0.8f, 1.0f), 1.0f},
        {"Neptune", 1.6f, 72.0f, 130.0f,    0.67f, 0, glm::vec3(0.4f,0.6f,1.0f), true, glm::vec3(0.4f, 0.5f, 1.0f), 1.2f}
    };
}

void Scene::init() {
    for(auto &p : texFiles){
        textures[p.first] = loadTexture(p.second);
        if(textures[p.first] == 0) {
            cerr << "Warning: texture " << p.second << " failed to load. Using 1x1 placeholder." << endl;
            unsigned char white[3] = {255,255,255};
            GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1,1,0,GL_RGB,GL_UNSIGNED_BYTE,white);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            textures[p.first] = t;
        }
    }

    std::cout << "Loaded textures:" << std::endl;
    for (const auto &kv : textures) {
        std::cout << "  " << kv.first << " -> ID " << kv.second << std::endl;
    }

    for(size_t i=0;i<planets.size();++i){
        string key = planets[i].name;
        transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return tolower(c); });
        auto it = textures.find(key);
        if(it != textures.end()) planets[i].texture = it->second;
        else {
            if(textures.count("sun")) planets[i].texture = textures["sun"];
            else planets[i].texture = 0;
        }
    }

    moonTexture = loadTexture("utils/textures/moon.jpeg");
    if(moonTexture == 0) {
        unsigned char moonCol[3] = {200,200,200};
        GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1,1,0,GL_RGB,GL_UNSIGNED_BYTE,moonCol);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        moonTexture = t;
    }

    setupOrbits();

    asteroidSystem = std::make_unique<AsteroidSystem>();
    asteroidSystem->init();

    dustSystem = std::make_unique<DustSystem>();
    dustSystem->init();

    // Initialize lens flare system
    lensFlareSystem = std::make_unique<LensFlareSystem>();
    lensFlareSystem->init();

    saturnRing = createRing(2.5f, 4.0f, 64);
    saturnRingTexture = loadTexture("utils/textures/saturn_ring.png");
    if (saturnRingTexture == 0) {
        const int TEX_SIZE = 256;
        std::vector<unsigned char> ringTex(TEX_SIZE * TEX_SIZE * 3);
        for (int y = 0; y < TEX_SIZE; y++) {
            for (int x = 0; x < TEX_SIZE; x++) {
                float dx = (x - TEX_SIZE / 2.0f);
                float dy = (y - TEX_SIZE / 2.0f);
                float dist = sqrt((dx*dx + dy*dy)) / (TEX_SIZE/2.0f);
                float ringPattern = (sin(dist * 20.0f) * 0.5f + 0.5f) * 0.7f;
                int idx = (y * TEX_SIZE + x) * 3;
                ringTex[idx+0] = (unsigned char)(200 * ringPattern + 55);
                ringTex[idx+1] = (unsigned char)(180 * ringPattern + 55);
                ringTex[idx+2] = (unsigned char)(160 * ringPattern + 55);
            }
        }
        glGenTextures(1, &saturnRingTexture);
        glBindTexture(GL_TEXTURE_2D, saturnRingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, ringTex.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    jupiterMoons = {
        {3.0f, 2.0f, 0.3f},
        {4.0f, 4.0f, 0.25f},
        {5.0f, 8.0f, 0.4f},
        {6.0f, 16.0f, 0.35f}
    };

    // Initialize atmosphere shader
    atmosphereShader = std::make_unique<Shader>(
        std::string("shader/atmosphere.vert"),
        std::string("shader/atmosphere.frag")
    );
}

void Scene::setupOrbits() {
    const int CIRCLE_SEGMENTS = 180;
    circleVerts.clear();
    for(int i=0;i<=CIRCLE_SEGMENTS;i++){
        float theta = 2.0f * M_PI * ((float)i / (float)CIRCLE_SEGMENTS);
        circleVerts.push_back(glm::vec3(cos(theta), 0.0f, sin(theta)));
    }
    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);
    glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVerts.size()*sizeof(glm::vec3), circleVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

glm::vec3 Scene::getPlanetPosition(int planetIndex, float simulationTime) {
    if(planetIndex < 0 || planetIndex >= (int)planets.size()) return glm::vec3(0.0f);

    Planet &p = planets[planetIndex];

    if(planetIndex == 0) return glm::vec3(0.0f); // Sun at origin

    if(p.orbitPeriod != 0.0f) {
        float orbitT = simulationTime / p.orbitPeriod;
        float angle = orbitT * 2.0f * (float)M_PI;
        float x = p.distance * cos(angle);
        float z = p.distance * sin(angle);
        return glm::vec3(x, 0.0f, z);
    }

    return glm::vec3(0.0f);
}

void Scene::renderAtmospheres(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos, Mesh &sphere, float simulationTime) {
    if(!atmosphereShader || !showAtmospheres) return;

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    atmosphereShader->use();
    atmosphereShader->setMat4("view", view);
    atmosphereShader->setMat4("projection", proj);
    atmosphereShader->setVec3("viewPos", camPos);

    glBindVertexArray(sphere.vao);

    for(size_t i=0; i<planets.size(); ++i) {
        Planet &p = planets[i];
        if(!p.hasAtmosphere) continue;

        glm::vec3 planetPos = getPlanetPosition(i, simulationTime);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, planetPos);

        float rotAngle = 0.0f;
        if(p.rotationPeriod != 0.0f) {
            float orbitSign = (p.orbitPeriod >= 0.0f) ? 1.0f : -1.0f;
            if(i == 0 && planets.size() > 1) {
                orbitSign = -((planets[1].orbitPeriod >= 0.0f) ? 1.0f : -1.0f);
            }
            rotAngle = orbitSign * (simulationTime / fabs(p.rotationPeriod)) * 360.0f;
            model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        model = glm::scale(model, glm::vec3(p.radius * 1.15f));

        atmosphereShader->setMat4("model", model);
        atmosphereShader->setVec3("atmosphereColor", p.atmosphereColor);
        glUniform1f(glGetUniformLocation(atmosphereShader->ID, "atmosphereIntensity"), p.atmosphereIntensity);

        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Scene::render(Shader &planetShader, const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos, const glm::vec3 &camFront, const glm::vec3 &camUp, Mesh &sphere, float simulationTime, float deltaTime, int screenWidth, int screenHeight) {
    planetShader.use();
    planetShader.setMat4("view", view);
    planetShader.setMat4("projection", proj);
    planetShader.setVec3("viewPos", camPos);

    // Draw orbits with dim color
    planetShader.use();
    planetShader.setInt("isSun", 0);
    glBindVertexArray(orbitVAO);
    for(size_t i=1;i<planets.size();++i){
        float r = planets[i].distance;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(r, 1.0f, r));
        planetShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures["sun"]);
        glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)circleVerts.size());
    }
    glBindVertexArray(0);

    // Draw planets
    glBindVertexArray(sphere.vao);
    for(size_t i=0;i<planets.size();++i){
        Planet &p = planets[i];
        glm::vec3 planetPos = getPlanetPosition(i, simulationTime);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, planetPos);

        float rotAngle = 0.0f;
        if(p.rotationPeriod != 0.0f) {
            float orbitSign = (p.orbitPeriod >= 0.0f) ? 1.0f : -1.0f;
            if(i == 0 && planets.size() > 1) {
                orbitSign = -((planets[1].orbitPeriod >= 0.0f) ? 1.0f : -1.0f);
            }
            rotAngle = orbitSign * (simulationTime / fabs(p.rotationPeriod)) * 360.0f;
            model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        model = glm::scale(model, glm::vec3(p.radius));
        planetShader.setMat4("model", model);

        glm::vec3 sunPos(0.0f);
        planetShader.setVec3("lightPos", sunPos);
        planetShader.setInt("isSun", (i==0) ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, p.texture);
        planetShader.setInt("texture1", 0);

        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

        // Moon for Earth
        if(i == 3) {
            float moonDist = 2.8f;
            float moonRadius = 0.35f;
            float moonOrbitPeriod = 3.0f;

            glm::vec3 earthPos = getPlanetPosition(3, simulationTime);
            float earthOrbitSign = (planets[3].orbitPeriod >= 0.0f) ? 1.0f : -1.0f;
            float moonT = simulationTime / moonOrbitPeriod;
            float moonAngle = -earthOrbitSign * moonT * 2.0f * (float)M_PI;
            float mx = earthPos.x + moonDist * cos(moonAngle);
            float mz = earthPos.z + moonDist * sin(moonAngle);

            glm::mat4 moonModel = glm::mat4(1.0f);
            moonModel = glm::translate(moonModel, glm::vec3(mx, 0.0f, mz));
            float moonRot = -earthOrbitSign * (simulationTime / 27.3f) * 360.0f;
            moonModel = glm::rotate(moonModel, glm::radians(moonRot), glm::vec3(0.0f, 1.0f, 0.0f));
            moonModel = glm::scale(moonModel, glm::vec3(moonRadius));

            planetShader.setMat4("model", moonModel);
            planetShader.setInt("isSun", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, moonTexture);
            planetShader.setInt("texture1", 0);
            glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
        }

        // Jupiter moons
        if(i == 5 && !jupiterMoons.empty()) {
            glm::vec3 jupiterPos = getPlanetPosition(5, simulationTime);
            for (const auto &moon : jupiterMoons) {
                float moonT = simulationTime / moon.orbitPeriod;
                float moonAngle = moonT * 2.0f * (float)M_PI;
                float mx = jupiterPos.x + moon.distance * cos(moonAngle);
                float mz = jupiterPos.z + moon.distance * sin(moonAngle);

                glm::mat4 moonModel = glm::mat4(1.0f);
                moonModel = glm::translate(moonModel, glm::vec3(mx, 0.0f, mz));
                moonModel = glm::rotate(moonModel, glm::radians(simulationTime * 10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                moonModel = glm::scale(moonModel, glm::vec3(moon.radius));
                planetShader.setMat4("model", moonModel);
                planetShader.setInt("isSun", 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, moonTexture);
                planetShader.setInt("texture1", 0);
                glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        // Saturn rings
        if(i == 6 && showRings) {
            glm::vec3 saturnPos = getPlanetPosition(6, simulationTime);
            glm::mat4 ringModel = glm::mat4(1.0f);
            ringModel = glm::translate(ringModel, saturnPos);
            ringModel = glm::rotate(ringModel, glm::radians(27.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            planetShader.setMat4("model", ringModel);
            planetShader.setInt("isSun", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, saturnRingTexture);
            planetShader.setInt("texture1", 0);
            glBindVertexArray(saturnRing.vao);
            glDrawElements(GL_TRIANGLES, saturnRing.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(sphere.vao);
        }
    }
    glBindVertexArray(0);

    // Render atmospheric glow
    renderAtmospheres(view, proj, camPos, sphere, simulationTime);

    // Asteroid belt
    if (asteroidSystem && showAsteroids) {
        asteroidSystem->render(simulationTime, sphere, planetShader);
    }

    // Space dust
    if (dustSystem && showDust) {
        dustSystem->update(deltaTime);
        dustSystem->render(view, proj, camFront, camUp);
    }

    // LENS FLARE - Render LAST so it appears on top
    if (lensFlareSystem && showLensFlare) {
        glm::vec3 sunPos = getPlanetPosition(0, simulationTime);
        lensFlareSystem->render(sunPos, view, proj, screenWidth, screenHeight);
    }
}

void Scene::cleanup() {
    if(orbitVBO) glDeleteBuffers(1, &orbitVBO);
    if(orbitVAO) glDeleteVertexArrays(1, &orbitVAO);
    if (asteroidSystem) { asteroidSystem->cleanup(); asteroidSystem.reset(); }
    if (dustSystem) { dustSystem->cleanup(); dustSystem.reset(); }
    if (lensFlareSystem) { lensFlareSystem->cleanup(); lensFlareSystem.reset(); }
    if (saturnRing.ebo) glDeleteBuffers(1, &saturnRing.ebo);
    if (saturnRing.vbo) glDeleteBuffers(1, &saturnRing.vbo);
    if (saturnRing.vao) glDeleteVertexArrays(1, &saturnRing.vao);
    if (saturnRingTexture) glDeleteTextures(1, &saturnRingTexture);
    atmosphereShader.reset();
}
