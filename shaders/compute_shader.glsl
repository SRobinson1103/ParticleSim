#version 450 core
layout(local_size_x = 256) in; // Must match C++ workGroupSize

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec3 color;
    float size;
    float isActive;
};

// Input/output buffers as arrays of Particle structs
layout(std430, binding = 0) buffer ParticleBufferIn { Particle particlesIn[]; };
layout(std430, binding = 1) buffer ParticleBufferOut { Particle particlesOut[]; };

// particle counts per cell. Flattened 2D grid: grid[gridCell.x + gridCell.y * gridWidth]
layout(std430, binding = 2) buffer GridBuffer { uint grid[]; };

// [gridIndex][particleIndex] = particleID
layout(std430, binding = 3) buffer GridParticlesBuffer { uint gridParticles[]; };

uniform vec2 mousePosition;         // Mouse position in clip space
uniform int mouseButton;            // Mouse button

uniform int createParticles;        // 1 = spawn particles, 0 = no
uniform int particleOffset;         // Offset in the buffer for new particles
uniform uint particlesToCreate;     // Number of particles to spawn per frame
uniform uint maxParticles;          // Max possible particles
uniform uint gridWidth;             // the width of the grid, = 2.0 / cellsize
uniform uint maxParticlesPerCell;   // 

uniform float minSpacing = 0.0005;  // Minimum distance between particles
uniform float cellSize;             // size of a single cell in the grid
uniform float gravity;              //
uniform float damping;              //
uniform float time;                 // For random
uniform float spawnRadius;          // Radius around mouse

uniform float groundY = -1.0;       // Y-coordinate of the ground (e.g., bottom of window)
uniform float groundBounce = 0.5;   // Bounce factor (0.0 = no bounce, 1.0 = perfect bounce)
uniform float groundFriction = 0.8; // Horizontal friction (0.0 = full stop, 1.0 = no friction)

// Random hash
// seed = particleID + time
float hash(uint seed)
{
    return fract(sin(float(seed) * 12.9898 + time) * 43758.5453);
}

void main() {
    uint particleID = gl_GlobalInvocationID.x;

    // Spawn new particles
    if (createParticles == 1 && particleID >= particleOffset && particleID < particleOffset + particlesToCreate)
    {
        // Generate random spawn position around mouse
        float angle = hash(particleID) * 6.283185307; // 0-2PI
        float radius = hash(particleID + 1) * 0.1;    // Spawn radius
        vec2 spawnPos = mousePosition + vec2(cos(angle), sin(angle)) * radius;

        bool canSpawn = true;
        ivec2 spawnCell = ivec2(spawnPos / cellSize);
        for (int dx = -1; dx <= 1; dx++)
        {
            for (int dy = -1; dy <= 1; dy++)
            {
                ivec2 neighborCell = spawnCell + ivec2(dx, dy);
                uint neighborIndex = neighborCell.x + neighborCell.y * gridWidth;
                for (uint i = 0; i < grid[neighborIndex]; i++)
                {
                    uint otherID = gridParticles[neighborIndex * maxParticlesPerCell + i];
                    Particle other = particlesIn[otherID];
                    if (distance(spawnPos, other.position) < minSpacing)
                    {
                        canSpawn = false;
                        break;
                    }
                }
            }
        }

        // Spawn particle if no overlap
        if (canSpawn)
        {
            particlesOut[particleID].position = spawnPos;
            particlesOut[particleID].velocity = vec2(cos(angle), sin(angle)) * 0.01;
            particlesOut[particleID].color = vec3(1.0, 0.0, 0.0);
            particlesOut[particleID].size = 0.01;
            particlesOut[particleID].isActive = 1.0;

            if (particlesOut[particleID].position.y - particlesOut[particleID].size < groundY)
            {
                particlesOut[particleID].position.y = groundY + particlesOut[particleID].size; // Clamp to ground level
            }
        }
        else
        {
            particlesOut[particleID].isActive = 0.0;
        }
        return;
    }

    // Update existing particle physics
    Particle pIn = particlesIn[particleID];
    if (pIn.isActive != 1.0)
    {
        particlesOut[particleID] = pIn;
        return;
    }

    pIn.velocity.y += gravity;
    pIn.velocity *= damping;
    pIn.position += pIn.velocity;

    // Ground collision
    float particleBottom = pIn.position.y - pIn.size;
    if (particleBottom < groundY)
    {
        // Correct position to stay above ground
        pIn.position.y = groundY + pIn.size;

        // Bounce vertically with damping
        pIn.velocity.y *= -groundBounce;

        // Apply horizontal friction
        pIn.velocity.x *= groundFriction;

        // Stop tiny movements
        if (length(pIn.velocity) < 0.0001)
        {
            pIn.velocity = vec2(0.0);
        }
    }

    // Update Grid
    ivec2 gridCell = ivec2(pIn.position / cellSize);
    uint gridIndex = gridCell.x + gridCell.y * gridWidth;

    // Atomically increment grid cell count and store particle ID
    uint index = atomicAdd(grid[gridIndex], 1);
    if (index < maxParticlesPerCell)
    {
        gridParticles[gridIndex * maxParticlesPerCell + index] = particleID;
    }

    // Collision Avoidance
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            ivec2 neighborCell = gridCell + ivec2(dx, dy);
            uint neighborIndex = neighborCell.x + neighborCell.y * gridWidth;

            // Iterate over particles in neighbor cell
            for (uint i = 0; i < grid[neighborIndex]; i++)
            {
                uint otherID = gridParticles[neighborIndex * maxParticlesPerCell + i];
                if (otherID == particleID) continue;

                Particle other = particlesIn[otherID];
                vec2 delta = pIn.position - other.position;
                float dist = length(delta);
                if (dist < minSpacing)
                {
                    // Repulsion force
                    pIn.velocity += normalize(delta) * 0.01;
                }
            }
        }
    }

    particlesOut[particleID] = pIn;

    /*
    // Apply gravity and damping
    pIn.velocity.y += -0.0001; // Small downward gravity
    pIn.velocity *= 0.99;      // Damping

    // Update position
    pIn.position += pIn.velocity;
    if(pIn.position.x >= 1.0)
    {
        pIn.position.x = 1.0;
        pIn.velocity.x = 0.0;
    }
    if(pIn.position.x <= -1.0)
    {
        pIn.position.x = -1.0;
        pIn.velocity.x = 0.0;
    }
    if(pIn.position.y >= 1.0)
    {
        pIn.position.y = 1.0;
        pIn.velocity.x = 0.0;
    }
    if(pIn.position.y <= -1.0)
    {
        pIn.position.y = -1.0;
        pIn.velocity.y = 0.0;
    }

    particlesOut[particleID] = pIn;
    */
}