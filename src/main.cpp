#include "ShaderUtil.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

// Callback to handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Function to process user input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

bool isLeftMousePressed = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            isLeftMousePressed = true;
        else if (action == GLFW_RELEASE)
            isLeftMousePressed = false;
    }
}

double mouseX, mouseY;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

int particleOffset = 0;
int maxParticles = 10000000;
int particlesToCreate = 100; // Number of particles to create per click (example)

// Define shader sources
const char* vertexShaderSource = R"(
    #version 450 core

    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec3 aColor;

    layout(std430, binding = 0) buffer ParticleBuffer {
        float particles[]; // Flat array containing particle data
    };

    out vec3 vertexColor;

    void main()
    {
        uint offset = gl_InstanceID * 8;
        vec2 position = vec2(particles[offset + 0], particles[offset + 1]);
        vec3 color = vec3(particles[offset + 2], particles[offset + 3], particles[offset + 4]);

        float velocity = length(vec2(particles[offset + 5], particles[offset + 6]));
        gl_PointSize = mix(1.0, 5.0, velocity); // Adjust point size based on velocity

        gl_Position = vec4(position, 0.0, 1.0);
        vertexColor = color;
    }
    )";


const char* fragmentShaderSource = R"(
    #version 450 core
    in vec3 vertexColor;

    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(vertexColor, 1.0);
    }
    )";

const char* computeShaderSource = R"(
    #version 450 core

    layout(local_size_x = 256, local_size_y = 1) in;

    layout(std430, binding = 0) buffer ParticleBuffer {
        float particles[]; // Flat array containing particle data
    };

    uniform vec2 mousePosition;
    uniform int createParticles;
    uniform int particlesToCreate;
    uniform int particleOffset;
    uniform int maxParticles;

    void main()
    {
        uint index = (gl_GlobalInvocationID.x + uint(particleOffset)) % uint(maxParticles);

        // Compute the offset for the current particle
        uint offset = index * 8;

        // If creating particles, initialize new particles near the mouse position
        if (createParticles == 1 && gl_GlobalInvocationID.x < uint(particlesToCreate))
        {
            float randOffsetX = (fract(sin(float(index * 1234)) * 43758.5453123) - 0.5) * 0.05; // Spread within -0.025 to 0.025
            float randOffsetY = (fract(sin(float(index * 5678)) * 43758.5453123) - 0.5) * 0.05; // Spread within -0.025 to 0.025


            particles[offset + 0] = mousePosition.x + randOffsetX; // x
            particles[offset + 1] = mousePosition.y + randOffsetY; // y
            particles[offset + 2] = 0.0; // r
            particles[offset + 3] = 0.0; // g
            particles[offset + 4] = 0.0; // b
            particles[offset + 5] = randOffsetX * 0.1; // vx
            particles[offset + 6] = randOffsetY * 0.1; // vy
            particles[offset + 7] = 1.0; // state (active)
        }
        else if (particles[offset + 7] == 1.0) // Update only active particles
        {
            // Read particle data
            float x = particles[offset + 0]; // x position
            float y = particles[offset + 1]; // y position
            float vx = particles[offset + 5]; // x velocity
            float vy = particles[offset + 6]; // y velocity

            float gravity = -0.001;
            vy += gravity;

            // Update position with velocity
            x += vx;
            y += vy;

            // Wrap around screen edges
            if (x > 1.0) x = -1.0;
            if (x < -1.0) x = 1.0;
            if (y > 1.0) y = -1.0;
            if (y < -1.0) y = 1.0;

            // Write back updated particle data
            particles[offset + 0] = x; // Update x position
            particles[offset + 1] = y; // Update y position
        }
    }
    )";

const char* initShaderSource = R"(
    #version 450 core

    layout(local_size_x = 256) in;

    layout(std430, binding = 0) buffer ParticleBuffer {
        float particles[]; // Flat array containing particle data
    };

    void main()
    {
        uint index = gl_GlobalInvocationID.x;
        uint offset = index * 8;

        // Randomized positions (-1.0 to 1.0)
        particles[offset + 0] = float(index % 100) / 50.0 - 1.0; // x
        particles[offset + 1] = float(index / 100) / 50.0 - 1.0; // y

        // Randomized colors (0.0 to 1.0)
        particles[offset + 2] = fract(sin(float(index)) * 43758.5453123); // r
        particles[offset + 3] = fract(sin(float(index + 1)) * 43758.5453123); // g
        particles[offset + 4] = fract(sin(float(index + 2)) * 43758.5453123); // b

        // Randomized velocities (-0.01 to 0.01)
        particles[offset + 5] = fract(sin(float(index + 3)) * 43758.5453123) * 0.02 - 0.01; // vx
        particles[offset + 6] = fract(sin(float(index + 4)) * 43758.5453123) * 0.02 - 0.01; // vy
        particles[offset + 7] = 0;
    }
    )";

// Main function
int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Compute Shader Particle Simulation", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Create shader program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLuint computeProgram = createComputeShaderProgram(computeShaderSource);
    GLuint initProgram = createComputeShaderProgram(initShaderSource);

    // Set up vertex array object (VAO)
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    int numParticles = 100000;

    // Create buffers for the compute shader
    GLuint ssbo[2];
    glGenBuffers(2, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    int workGroupSize = 256; // Must match local_size_x
    int numGroups = (numParticles + workGroupSize - 1) / workGroupSize;

    // Bind the first SSBO to the initialization program
    glUseProgram(initProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]);
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure buffer writes are complete
    glDeleteProgram(initProgram);

    // Configure attributes for rendering
    glBindBuffer(GL_ARRAY_BUFFER, ssbo[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float))); // Color
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int currentSSBO = 0;
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Use the current SSBO for compute shader updates
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[currentSSBO]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1 - currentSSBO]);

        float normalizedMouseX = (mouseX / 800.0) * 2.0f - 1.0f;
        float normalizedMouseY = -((mouseY / 600.0) * 2.0f - 1.0f);

        if (isLeftMousePressed)
        {
            particleOffset = (particleOffset + particlesToCreate) % maxParticles;
        }

        // Dispatch compute shader
        glUseProgram(computeProgram);
        // Pass uniforms
        glUniform2f(glGetUniformLocation(computeProgram, "mousePosition"), normalizedMouseX, normalizedMouseY);
        glUniform1i(glGetUniformLocation(computeProgram, "createParticles"), isLeftMousePressed ? 1 : 0);
        glUniform1i(glGetUniformLocation(computeProgram, "particleOffset"), particleOffset);
        glUniform1i(glGetUniformLocation(computeProgram, "maxParticles"), maxParticles);
        glUniform1i(glGetUniformLocation(computeProgram, "particlesToCreate"), particlesToCreate);

        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // Render using the updated SSBO
        glBindBuffer(GL_ARRAY_BUFFER, ssbo[1 - currentSSBO]);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render particles as points
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, numParticles);

        currentSSBO = 1 - currentSSBO;

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(2, ssbo);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(computeProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
