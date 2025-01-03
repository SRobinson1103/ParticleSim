#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

layout(std430, binding = 0) buffer ParticleBuffer {
    float particles[]; // Flat array containing particle data
};

out vec3 particleColor;    // Output color for the geometry shader
out vec2 particlePosition; // Output position for the geometry shader

void main()
{
    uint offset = gl_InstanceID * 10;

    if (particles[offset + 7] != 1.0)
    {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0); // Off-screen position
        return;
    }

    //vec2 position = vec2(particles[offset + 0], particles[offset + 1]);
    //vec3 color = vec3(particles[offset + 2], particles[offset + 3], particles[offset + 4]);

    particlePosition = vec2(particles[offset + 0], particles[offset + 1]);
    particleColor = vec3(particles[offset + 2], particles[offset + 3], particles[offset + 4]);
}