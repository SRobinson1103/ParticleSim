#version 450 core

layout(points) in;                            // Input is a single point per particle
layout(triangle_strip, max_vertices = 4) out; // Output is a triangle strip forming a square

in vec3 particleColor[];                      // Color passed from the vertex shader
in vec2 particlePosition[];                   // Position passed from the vertex shader

out vec3 vertexColor;                         // Output color to the fragment shader

void main()
{
    vec2 center = particlePosition[0]; // Center of the square
    vec3 color = particleColor[0];     // Color of the particle

    float size = 0.01; // Half extents of the square

    // Define square vertices
    vec2 offsets[4] = vec2[](
        vec2(-size, -size), // Bottom-left
        vec2(size, -size),  // Bottom-right
        vec2(-size, size),  // Top-left
        vec2(size, size)    // Top-right
    );

    // Emit vertices for the square
    for (int i = 0; i < 4; i++)
    {
        gl_Position = vec4(center + offsets[i], 0.0, 1.0); // Compute vertex position
        vertexColor = color;                               // Pass color to fragment shader
        EmitVertex();
    }
    EndPrimitive();
}
