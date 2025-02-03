#version 450 core
layout(local_size_x = 256) in; // Must match C++ workGroupSize

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec3 color;
    float size;
    float isActive; // 1.0 = active,  0.0 = inactive
    float resting;  // 1.0 = resting, 0.0 = not resting
};

// Input/output buffers as arrays of Particle structs
layout(std430, binding = 0) buffer ParticleBufferIn { Particle particlesIn[]; };
layout(std430, binding = 1) buffer ParticleBufferOut { Particle particlesOut[]; };

// Number of particles per cell.
layout(std430, binding = 2) buffer GridBuffer { uint grid[]; };

// Flattened: [cellIndex * maxParticlesPerCell + particleIndex]
layout(std430, binding = 3) buffer GridParticlesBuffer { uint gridParticles[]; };

uniform vec2 mousePosition;           // Mouse position in clip space
uniform int mouseButton;              // Mouse button

uniform int createParticles;          // 1 = spawn particles, 0 = no
uniform int particleOffset;           // Offset in the buffer for new particles
uniform uint particlesToCreate;       // Number of particles to spawn per frame
uniform uint maxParticles;            // Max possible particles
uniform uint gridWidth;               // the width of the grid, = 2.0 / cellsize
uniform uint maxParticlesPerCell = 2; // 

uniform float minSpacing = 0.01;      // Minimum distance between particles
uniform float cellSize;               // size of a single cell in the grid
uniform float gravity;                //
uniform float damping;                //
uniform float time;                   // For random
uniform float spawnRadius;            // Radius around mouse

uniform float groundY = -1.0;         // Y-coordinate of the ground (e.g., bottom of window)
uniform float groundBounce = 1.0;     // Bounce factor (0.0 = no bounce, 1.0 = perfect bounce)
uniform float groundFriction = 0.8;   // Horizontal friction (0.0 = full stop, 1.0 = no friction)

// Random hash
// seed = particleID + time
float hash(uint seed)
{
    return fract(sin(float(seed) * 12.9898 + time) * 43758.5453);
}

void main()
{
    uint particleID = gl_GlobalInvocationID.x;

    // ----------- SPAWN PARTICLES -----------
    if (createParticles == 1 && particleID >= particleOffset && particleID < particleOffset + particlesToCreate)
    {
        // Generate random spawn position around mouse
        float angle = hash(particleID) * 6.283185307; // 0-2PI
        float radius = hash(particleID + 1) * 0.1;    // Spawn radius
        vec2 spawnPos = mousePosition + vec2(cos(angle), sin(angle)) * radius;

        bool canSpawn = true;
        //ivec2 spawnCell = ivec2((spawnPos + vec2(1.0, 1.0)) / cellSize); //convert position to a cell in the grid
        //for (int dx = -1; dx <= 1; dx++)
        //{
        //    for (int dy = -1; dy <= 1; dy++)
        //    {
        //        ivec2 neighborCell = spawnCell + ivec2(dx, dy);
        //        uint neighborIndex = neighborCell.x + neighborCell.y * gridWidth;
        //        // Spawn the particle only if particles in the current and neighboring cells
        //        // are above a certain distance away
        //        for (uint i = 0; i < grid[neighborIndex]; i++)
        //        {
        //            uint otherID = gridParticles[neighborIndex * maxParticlesPerCell + i];
        //            Particle other = particlesIn[otherID];
        //            if (distance(spawnPos, other.position) < minSpacing)
        //            {
        //                canSpawn = false;
        //                break;
        //            }
        //        }
        //    }
        //}

        // Below ground
        if (particlesOut[particleID].position.y - particlesOut[particleID].size < groundY)
            canSpawn = false;

        // Spawn particle if no overlap and above ground
        if (canSpawn)
        {
            particlesOut[particleID].position = spawnPos;
            particlesOut[particleID].velocity = vec2(cos(angle), sin(angle)) * 0.01;
            particlesOut[particleID].color = vec3(1.0, 0.0, 0.0);
            particlesOut[particleID].size = cellSize;
            particlesOut[particleID].isActive = 1.0;
            particlesOut[particleID].resting = 0.0;
        }
        else
        {
            // Creates gaps in the buffer
            particlesOut[particleID].isActive = 0.0;
        }
        return;
    }

    Particle pIn = particlesIn[particleID];

    // Don't do anything with inactive particles
    if (pIn.isActive != 1.0)
    {
        particlesOut[particleID] = pIn;
        return;
    }

    // ----------- UPDATE PHYSYCS -----------
    pIn.velocity.y += gravity;
    pIn.velocity *= damping;
    pIn.position += pIn.velocity;

    // Ground collision
    float particleBottom = pIn.position.y - pIn.size * 0.5;
    if (particleBottom <= groundY)
    {
        // Correct position to stay above ground
        pIn.position.y = groundY + pIn.size * 0.5;
        // Bounce vertically
        pIn.velocity.y *= -groundBounce;
        // Apply horizontal friction
        pIn.velocity.x *= groundFriction;

        if (abs(pIn.velocity.y) < 0.01)
        {
            pIn.velocity.y = 0.0;
            pIn.resting = 1.0;
        }
    }

    // apply updates
    particlesOut[particleID] = pIn;

    // ----------- BUILD GRID -----------
    ivec2 gridCell = ivec2((pIn.position + vec2(1.0)) / cellSize);
    uint gridIndex = uint(gridCell.x + gridCell.y * gridWidth);

    // Atomically increment the cell count and store this particle's id
    uint index = atomicAdd(grid[gridIndex], 1);
    if (index < maxParticlesPerCell)
    {
        gridParticles[gridIndex * maxParticlesPerCell + index] = particleID;
    }
}