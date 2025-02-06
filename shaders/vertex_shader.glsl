#version 450 core

struct Particle
{
    uint id;
    vec2 position;
    vec2 velocity;
    vec3 color;
    float size;
    float isActive;
    float resting;
};

layout(std430, binding = 0) buffer ParticleBuffer
{
    Particle particles[];
};

layout(location = 0) in vec2 aPos;

out vec3 fragColor;

void main()
{
    int instanceID = gl_InstanceID;
    Particle p = particles[instanceID];

    if (p.isActive != 1.0)
    {
        gl_Position = vec4(0.0); // Discard inactive particles
        return;
    }

    // Transform quad vertex by particle size/position
    gl_Position = vec4(p.position + (aPos * p.size), 0.0, 1.0);

    fragColor = p.color;
}