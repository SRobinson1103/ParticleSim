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

bool isMousePressed = false;
int mouseButton = 0;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouseButton = 1;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            mouseButton = 2;
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouseButton = 3;

        if (action == GLFW_PRESS)
            isMousePressed = true;
        else if (action == GLFW_RELEASE)
            isMousePressed = false;
    }
}

double mouseX, mouseY;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

int particleOffset = 0;
int particlesToCreate = 10;

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
    std::string vertexShaderSource = loadShaderFromFile("..\\shaders\\vertex_shader.glsl");
    std::string fragmentShaderSource = loadShaderFromFile("..\\shaders\\fragment_shader.glsl");
    std::string geometryShaderSource = loadShaderFromFile("..\\shaders\\geometry_shader.glsl");
    std::string computeShaderSource = loadShaderFromFile("..\\shaders\\compute_shader.glsl");
    const char* vertexSourceCStr = vertexShaderSource.c_str();
    const char* fragmentSourceCStr = fragmentShaderSource.c_str();
    const char* geometrySourceCStr = geometryShaderSource.c_str();
    const char* computeSourceCStr = computeShaderSource.c_str();

    GLuint shaderProgram = createShaderProgram(vertexSourceCStr, geometrySourceCStr, fragmentSourceCStr);
    GLuint computeProgram = createComputeShaderProgram(computeSourceCStr);

    // Set up vertex array object (VAO)
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    int maxParticles = 100000;

    // Create buffers for the compute shader
    GLuint ssbo[2];
    glGenBuffers(2, ssbo);
   // std::vector<float> initialData(maxParticles * 8, 0.0f);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * 10 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * 10 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, ssbo[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(2 * sizeof(float))); // Color
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int currentSSBO = 0;
    int workGroupSize = 256; // Must match local_size_x
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        if (isMousePressed)
        {
            particleOffset = std::min((particleOffset + particlesToCreate), maxParticles);
        }

        float normalizedMouseX = (mouseX / 800.0) * 2.0f - 1.0f;
        float normalizedMouseY = -((mouseY / 600.0) * 2.0f - 1.0f);

        // Use the current SSBO for compute shader updates
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[currentSSBO]); //reads from this
        // use the next SSBO for writing
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1 - currentSSBO]); //writes to this

        // Dispatch compute shader
        glUseProgram(computeProgram);
        // Pass uniforms
        glUniform2f(glGetUniformLocation(computeProgram, "mousePosition"), normalizedMouseX, normalizedMouseY);
        glUniform1i(glGetUniformLocation(computeProgram, "mouseButton"), mouseButton);
        glUniform1i(glGetUniformLocation(computeProgram, "createParticles"), isMousePressed ? 1 : 0);
        glUniform1i(glGetUniformLocation(computeProgram, "particleOffset"), particleOffset - particlesToCreate);
        glUniform1i(glGetUniformLocation(computeProgram, "maxParticles"), maxParticles);
        glUniform1i(glGetUniformLocation(computeProgram, "particlesToCreate"), particlesToCreate);
        glUniform1f(glGetUniformLocation(computeProgram, "gravity"), -0.000f);
        glUniform1f(glGetUniformLocation(computeProgram, "damping"), 0.99f);

        int activeParticles = particleOffset;
        int activeGroups = (activeParticles + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(activeGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, ssbo[1 - currentSSBO]);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render particles as points
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, activeParticles);

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
