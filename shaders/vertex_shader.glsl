#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

layout(std430, binding = 0) buffer ParticleBuffer {
    float particles[]; // Flat array containing particle data
};

out vec3 vertexColor;

void main()
{
    uint offset = gl_InstanceID * 10;

    if (particles[offset + 7] != 1.0)
    {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0); // Off-screen position
        return;
    }

    vec2 position = vec2(particles[offset + 0], particles[offset + 1]);
    vec3 color = vec3(particles[offset + 2], particles[offset + 3], particles[offset + 4]);

    float velocity = length(vec2(particles[offset + 5], particles[offset + 6]));
    gl_PointSize = mix(1.0, 5.0, velocity); // Adjust point size based on velocity

    // Triangle vertices around the particle's position
    float size = 0.02; // Size of the triangle
    vec2 vertexOffset;

    if (gl_VertexID == 0) {
        vertexOffset = vec2(-size, -size);
    } else if (gl_VertexID == 1) {
        vertexOffset = vec2(size, -size);
    } else if (gl_VertexID == 2) {
        vertexOffset = vec2(0.0, size);
    }

    gl_Position = vec4(position + vertexOffset, 0.0, 1.0);
    vertexColor = color;
}