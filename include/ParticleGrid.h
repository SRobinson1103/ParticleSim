#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

int particleOffset = 0;
constexpr float cellSize = 0.01f;
constexpr glm::uint gridWidth = static_cast<glm::uint>(2.0f / cellSize);
constexpr glm::uint maxParticles = gridWidth * gridWidth*10;
glm::uint particlesToCreate = 100;
glm::uint maxParticlesPerCell = 2;
float spawnRadius = 0.01f;
float gravity = -0.001f;
float damping = 0.99f;

struct Particle
{
    glm::uint id;
    glm::vec2 position; //[0, 1]
    glm::vec2 velocity; //[2, 3]
    glm::vec3 color;    //[4, 5, 6]
    float size;         //[7]
    float active;       //[8]
    float resting;      //[9]
};

// Quad vertices (x,y)
const float quadVertices[] =
{
    -0.5f, -0.5f,
     0.5f, -0.5f,
    -0.5f,  0.5f,
     0.5f,  0.5f
};

#endif