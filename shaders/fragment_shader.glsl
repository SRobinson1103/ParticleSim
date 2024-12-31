#version 460 core

in vec3 vertexColor; // Received from vertex shader
out vec4 FragColor; // Output color

void main()
{
    FragColor = vec4(vertexColor, 1.0); // RGBA
}
