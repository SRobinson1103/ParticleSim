 #version 450 core

layout(local_size_x = 256, local_size_y = 1) in;

layout(std430, binding = 0) buffer ReadBuffer {
    float particles[];
};

layout(std430, binding = 1) buffer WriteBuffer {
    float newParticles[];
};

uniform vec2 mousePosition;
uniform int createParticles;
uniform int particlesToCreate;
uniform int particleOffset;
uniform int maxParticles;
uniform float gravity;
uniform float damping;

void main()
{
    uint index = gl_GlobalInvocationID.x;
    uint offset = index * 10; // Calculate the buffer offset for the particle

    if (createParticles == 1 && gl_GlobalInvocationID.x > uint(particleOffset) && gl_GlobalInvocationID.x <= uint(particleOffset + particlesToCreate))
    {
        float randOffsetX = (fract(sin(float(gl_GlobalInvocationID.x * 1234)) * 43758.5453123) - 0.5) * 0.05; // Spread within -0.025 to 0.025
        float randOffsetY = (fract(sin(float(gl_GlobalInvocationID.x * 5678)) * 43758.5453123) - 0.5) * 0.05; // Spread within -0.025 to 0.025

        newParticles[offset + 0] = mousePosition.x + randOffsetX; // x
        newParticles[offset + 1] = mousePosition.y + randOffsetY; // y
        newParticles[offset + 2] = mousePosition.x; // r
        newParticles[offset + 3] = mousePosition.y; // g
        newParticles[offset + 4] = mousePosition.x * mousePosition.y * 4; // b
        newParticles[offset + 5] = randOffsetX * 0.1; // vx
        newParticles[offset + 6] = randOffsetY * 0.1; // vy
        newParticles[offset + 7] = 1.0; // state (active)
        newParticles[offset + 8] = 0.1;
    }    
    else if (particles[offset + 7] == 1.0) // Update only active particles
    {
        // Read particle data
        float x = particles[offset + 0]; // x position
        float y = particles[offset + 1]; // y position
        float vx = particles[offset + 5]; // x velocity
        float vy = particles[offset + 6]; // y velocity

        vy += gravity;
        vx *= damping;
        vy *= damping;

        // Update position with velocity
        x += vx;
        y += vy;

        // Wrap around screen edges
        if (x > 1.0) x = -1.0;
        if (x < -1.0) x = 1.0;
        if (y > 1.0) y = -1.0;
        if (y < -1.0) y = 1.0;

        // Write back updated particle data
        newParticles[offset + 0] = x; // Update x position
        newParticles[offset + 1] = y; // Update y position
        newParticles[offset + 5] = vx;
        newParticles[offset + 6] = vy;
        newParticles[offset + 7] = 1.0;
    }

    for (uint i = 0; i < particleOffset + particlesToCreate; i++)
    {
        if (i == index) continue; // Skip self

        float x = particles[offset + 0];
        float y = particles[offset + 1];
        float vx = particles[offset + 5];
        float vy = particles[offset + 6];

        uint otherOffset = i * 10;
        float otherX = particles[otherOffset + 0];
        float otherY = particles[otherOffset + 1];
        float otherRadius = particles[otherOffset + 8]; // Assuming radius is stored at offset + 8

        // Distance between particles
        float dx = otherX - x;
        float dy = otherY - y;
        float distance = sqrt(dx * dx + dy * dy);
        float radius = particles[offset + 8];

        if (distance < (radius + otherRadius))
        {
            float otherVx = particles[otherOffset + 5];
            float otherVy = particles[otherOffset + 6];

            // Unit normal and tangent vectors
            vec2 normal = normalize(vec2(dx, dy));
            vec2 tangent = vec2(-normal.y, normal.x);

            // Project velocities onto normal and tangent
            float v1n = dot(normal, vec2(vx, vy));
            float v2n = dot(normal, vec2(otherVx, otherVy));
            float v1t = dot(tangent, vec2(vx, vy));
            float v2t = dot(tangent, vec2(otherVx, otherVy));

            // Elastic collision (swap normal components)
            float v1nAfter = v2n;
            float v2nAfter = v1n;

            // Reconstruct velocities
            vec2 v1nVec = v1nAfter * normal;
            vec2 v1tVec = v1t * tangent;
            vec2 v2nVec = v2nAfter * normal;
            vec2 v2tVec = v2t * tangent;

            vec2 newVelocity1 = v1nVec + v1tVec;
            vec2 newVelocity2 = v2nVec + v2tVec;

            // Apply new velocities
            newParticles[offset + 5] = newVelocity1.x;
            newParticles[offset + 6] = newVelocity1.y;
            newParticles[otherOffset + 5] = newVelocity2.x;
            newParticles[otherOffset + 6] = newVelocity2.y;
        }
    }
}