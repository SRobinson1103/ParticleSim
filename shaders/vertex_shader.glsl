#version 460 core

layout(location = 0) in vec2 aPos; // Vertex position
layout(location = 1) in vec3 aColor; // Vertex color

out vec3 vertexColor; // Pass color to fragment shader

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0); // Convert 2D to 3D
    vertexColor = aColor; // Pass color to fragment shader
}
