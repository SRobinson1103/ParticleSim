#version 450 core
layout(local_size_x = 256) in; // Must match C++ workGroupSize

struct Particle
{
    uint id;
    vec2 position;
    vec2 velocity;
    vec3 color;
    float size;
    float isActive; // 1.0 = active,  0.0 = inactive
    float resting;  // 1.0 = resting, 0.0 = not resting
};

layout(std430, binding = 0) buffer ParticleBufferIn { Particle particlesIn[]; };
// output buffer from previous compute shader
layout(std430, binding = 1) buffer ParticleBufferOut { Particle particles[]; };

// Number of particles per cell.
layout(std430, binding = 2) buffer GridBuffer { uint grid[]; };

// Flattened: [cellIndex * maxParticlesPerCell + particleIndex]
layout(std430, binding = 3) buffer GridParticlesBuffer { uint gridParticles[]; };

uniform float cellSize;
uniform uint gridWidth;
uniform uint maxParticlesPerCell;
uniform float collisionResponseFactor = 0.5;
uniform float restingThreshold = 0.002;

void main()
{
    uint particleID = gl_GlobalInvocationID.x;
    Particle previous = particlesIn[particleID];
    Particle p = particles[particleID];

    if (p.isActive != 1.0) return;

    // Remap the particle position from simulation space [-1,1] to grid-space.
    ivec2 gridCell = ivec2((p.position + vec2(1.0, 1.0)) / cellSize);
    ivec2 lastGridCell = ivec2((previous.position + vec2(1.0, 1.0)) / cellSize);

    bool supportFound = false;
    bool aboveAnother = false;

    // The cell is at the bottom
    if (p.position.y - p.size * 0.5 <= -1.0)
    {
        p.position.y = -1.0 + p.size * 0.5;
        p.resting = 1.0;
        p.velocity.y = 0.0;
        supportFound = true;
    }
    else
    {
        //loop through all particles in the cell below
        ivec2 neighborCell = gridCell + ivec2(0, -1);
        uint neighborCellIndex = uint(neighborCell.x + neighborCell.y * gridWidth);
        for (uint i = 0; i < grid[neighborCellIndex]; i++)
        {
            aboveAnother = true;

            //get the particle
            uint neighborID = gridParticles[neighborCellIndex * maxParticlesPerCell + i];
            Particle other = particles[neighborID];
            if (other.isActive != 1.0)
                continue;

            // Calculate where p should be if it were resting on top of the other particle.
            //float expectedY = other.position.y + (other.size + p.size) * 0.5;
            float expectedY = other.position.y + other.size;
            // Compare p's bottom with the expected top of the resting neighbor.
            float pBottom = p.position.y - p.size * 0.5;
            float otherTop = other.position.y + other.size * 0.5;
            float dist = pBottom - otherTop;
            if (dist <= 0.0) //particles has overlapped the lower particle
            {
                p.position.y = expectedY;// Snap the particle's position so that it sits exactly on top.
                p.velocity.y = 0.0;
                p.resting = 1.0;
                supportFound = true;
                break;
            }
            else if (dist <= restingThreshold)
            {
                p.position = previous.position;
                p.velocity.y = 0.0;
                p.resting = 1.0;
                supportFound = true;
                break;
            }
        }
    }

    if(supportFound)
    {
        p.color = vec3(1.0, 0.0, 0.0);
        particles[particleID] = p;
        return;
    }
    else if (aboveAnother)
    {
        p.color = vec3(0.0, 1.0, 0.0);
    }
    else
    {
        p.color = vec3(1.0, 1.0, 1.0);
    }

    p.resting = 0.0;
    particles[particleID] = p;

    // Accumulate a correction vector from all collisions in nearby cells.
    //vec2 correction = vec2(0.0);
    //int correctionCount = 0;
    
    // Iterate over the 3x3 neighborhood around the current grid cell.
    //for (int dx = -1; dx <= 1; dx++)
    //{
    //    for (int dy = -1; dy <= 1; dy++)
    //    {
    //        ivec2 neighborCell = gridCell + ivec2(dx, dy);
    //
    //        // Check that the neighbor cell is within grid bounds.
    //        if (neighborCell.x < 0 || neighborCell.y < 0 || neighborCell.x >= gridWidth || neighborCell.y >= gridWidth)
    //        {
    //            continue;
    //        }
    //
    //        uint cellIndex = uint(neighborCell.x + neighborCell.y * gridWidth);
    //        // Loop through all particles in this cell.
    //        for (uint i = 0; i < grid[cellIndex]; i++)
    //        {
    //            uint otherID = gridParticles[cellIndex * maxParticlesPerCell + i];
    //            // Skip self
    //            if (otherID == particleID)
    //                continue;
    //            Particle other = particles[otherID];
    //            if (other.isActive != 1.0)
    //                continue;
    //
    //            // Compute the minimum allowed distance between the two particles.
    //            // Use half the sum of their sizes as an approximate radius.
    //            float minDist = (p.size + other.size) * 0.5;
    //            vec2 delta = p.position - other.position;
    //            float dist = length(delta);
    //            // If the distance is less than the minimum, there's an overlap.
    //            if (dist < minDist && dist > 0.0)
    //            {
    //                float penetration = (minDist - dist);
    //                // Calculate the normalized direction vector.
    //                vec2 normal = normalize(delta);
    //                // Accumulate the correction vector. You can scale the penetration
    //                // by a factor to control the "stiffness" of the collision.
    //                correction += normal * penetration;
    //                correctionCount++;
    //            }
    //        }
    //    }
    //}
    //
    //// If we found any overlapping neighbors, average the correction and apply it.
    //if (correctionCount > 0)
    //{
    //    correction /= float(correctionCount);
    //    // Apply a response factor to control how strongly the particle is pushed.
    //    p.position += correction * collisionResponseFactor;
    //
    //    // Optionally, you might want to adjust the velocity.
    //    // For instance, remove the component of velocity in the direction of correction.
    //    float vn = dot(p.velocity, normalize(correction));
    //    if (vn > 0.0)
    //    {
    //        p.velocity -= normalize(correction) * vn;
    //    }
    //}
    //
    ////Write the updated particle state back into the global buffer.
    //particles[particleID] = p;
}
