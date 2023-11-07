#include <SDL.h>
#include <SDL_events.h>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <sstream>
#include <vector>
#include "color.h"
#include "framebuffer.h"
#include "uniforms.h"
#include "shaders.h"
#include "fragment.h"
#include "triangle.h"
#include "camera.h"
#include "ObjLoader.h"
#include "noise.h"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Color currentColor;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    setupNoise();

    return true;
}

void setColor(const Color& color) {
    currentColor = color;
}

void render(const std::vector<glm::vec3>& VBO, const Uniforms& uniforms) {
    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices(VBO.size() / 3);
    for (size_t i = 0; i < VBO.size() / 3; ++i) {
        Vertex vertex = { VBO[i * 3], VBO[i * 3 + 1], VBO[i * 3 + 2] };
        transformedVertices[i] = vertexShader(vertex, uniforms);
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledVertices(transformedVertices.size() / 3);
    for (size_t i = 0; i < transformedVertices.size() / 3; ++i) {
        Vertex edge1 = transformedVertices[3 * i];
        Vertex edge2 = transformedVertices[3 * i + 1];
        Vertex edge3 = transformedVertices[3 * i + 2];
        assembledVertices[i] = { edge1, edge2, edge3 };
    }

    // 3. Rasterization
    std::vector<Fragment> fragments;
    for (size_t i = 0; i < assembledVertices.size(); ++i) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                assembledVertices[i][0],
                assembledVertices[i][1],
                assembledVertices[i][2]
        );

        fragments.insert(fragments.end(), rasterizedTriangle.begin(), rasterizedTriangle.end());
    }

    // 4. Fragment Shader
    for (size_t i = 0; i < fragments.size(); ++i) {
        Fragment& fragment = fragments[i];

        if (uniforms.objectType == ObjectType::SUN) {
            sun(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::EARTH) {
            earth(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::JUPITER) {
            jupiter(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::MOON) {
            moon(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::GASEOSO) {
            planetaGaseoso(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::ANTITIERRA) {
            antiTierra(fragment, uniforms.time);
        } else if (uniforms.objectType == ObjectType::PLANETACOLOR) {
            planetaColor(fragment, uniforms.time);
        }

        point(fragment);
    }
}

glm::mat4 createViewportMatrix(size_t screenWidth, size_t screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

struct Planet {
    ObjectType type;
    float orbitRadius;
    float scaleFactor;
    float orbitSpeed;
    float orbitAngle;
};

std::vector<Planet> planets;
int currentPlanet = 0;

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> texCoords;
    std::vector<Face> faces;
    std::vector<glm::vec3> vertexBufferObject;

    loadOBJ("../models/sphere.obj", vertices, normals, texCoords, faces);

    for (const auto& face : faces)
    {
        for (int i = 0; i < 3; ++i)
        {
            // Get the vertex position
            glm::vec3 vertexPosition = vertices[face.vertexIndices[i]];

            // Get the normal for the current vertex
            glm::vec3 vertexNormal = normals[face.normalIndices[i]];

            // Get the texture for the current vertex
            glm::vec3 vertexTexture = texCoords[face.texIndices[i]];

            // Add the vertex position and normal to the vertex array
            vertexBufferObject.push_back(vertexPosition);
            vertexBufferObject.push_back(vertexNormal);
            vertexBufferObject.push_back(vertexTexture);
        }
    }

    Uniforms uniforms;

    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);

    glm::vec3 translationVector(0.0f, 0.0f, 0.0f);
    float a = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Rotate around the Y-axis
    glm::vec3 scaleFactor(1.0f, 1.0f, 1.0f);

    // Initialize a Camera object
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 1.5f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Projection matrix
    float fovInDegrees = 45.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT); // Assuming a screen resolution of 800x600
    float nearClip = 0.1f;
    float farClip = 100.0f;
    uniforms.projection = glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);

    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);
    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";
    int speed = 10;

    planets.push_back({ ObjectType::SUN, 0.1f, 0.15f, 0.0f, 0.0f });
    planets.push_back({ ObjectType::JUPITER, 0.25f, 0.06f,0.07f, 0.0f });
    planets.push_back({ ObjectType::GASEOSO, 0.35f, 0.06f,0.05f, 0.0f });
    planets.push_back({ ObjectType::EARTH, 0.45f, 0.08f,0.03f, 0.0f });
    planets.push_back({ ObjectType::PLANETACOLOR, 0.56f, 0.08f,0.01f, 0.0f });

    bool running = true;
    while (running) {
        frame += 1;
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        // Cycle to next planet camera
                        currentPlanet = (currentPlanet + 1) % planets.size();
                        break;

                    case SDLK_a:
                        camera.cameraPosition.x -= 0.1f;
                        break;

                    case SDLK_d:
                        camera.cameraPosition.x += 0.1f;
                        break;

                    case SDLK_w:
                        camera.cameraPosition.y += 0.1f;
                        break;

                    case SDLK_s:
                        camera.cameraPosition.y -= 0.1f;
                        break;

                    case SDLK_UP:
                        camera.cameraPosition.z -= 0.1f;
                        break;

                    case SDLK_DOWN:
                        camera.cameraPosition.z += 0.1f;
                        break;

                    case SDLK_r:
                        camera.cameraPosition = glm::vec3(0.0f, 0.0f, 1.5f);
                        break;
                }
            }
        }

        // Create the view matrix using the Camera object
        uniforms.view = glm::lookAt(
                camera.cameraPosition, // The position of the camera
                camera.targetPosition, // The point the camera is looking at
                camera.upVector        // The up vector defining the camera's orientation
        );

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        clearFramebuffer();

        for (auto& planet : planets) {
            uniforms.objectType = planet.type;
            uniforms.time = SDL_GetTicks() / 1000.0f;

            glm::vec3 systemOffset(-0.1f, 0.0f, 0.0f);

            float orbitAngle = planet.orbitAngle;
            float orbitRadius = planet.orbitRadius;
            float orbitX = orbitRadius * cos(orbitAngle);
            float orbitY = orbitRadius * sin(orbitAngle);

            // Calculate the model matrix
            glm::mat4 translate = glm::translate(glm::mat4(1.0f), systemOffset);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(planet.orbitAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(orbitX, orbitY, 0));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(planet.scaleFactor));
            glm::mat4 model = translate * translation * rotation * scale;

            uniforms.model = model;

            // Render the model
            render(vertexBufferObject, uniforms);

            // Update the planet's orbit angle
            planet.orbitAngle += planet.orbitSpeed;
        }

        renderBuffer(renderer);

        frameTime = SDL_GetTicks() - frameStart;

        // Calculate frames per second and update window title
        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;  // Milliseconds to seconds
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
