#include "ShaderUtil.hpp"
#include "GridLines.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <vector>

bool initOpenGL();
void loadShaders();
void initializeParticleBuffers();
void renderLoop();
void cleanup();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

void dispatchFirstCompute(int activeGroups);
void dispatchSecondCompute(int activeGroups);

GLFWwindow* window;
int windowWidth = 800;
int windowHeight = 600;
GLuint shaderProgram, computeProgramFirst, computeProgramSecond;
double mouseX, mouseY;
bool isMousePressed = false;
int mouseButton = 0;

GLuint quadVAO, quadVBO;
GLuint ssbo[2];
GLuint gridSSBO, gridParticlesSSBO;

int particleOffset = 0;
constexpr float cellSize = 0.004f;
constexpr glm::uint gridWidth = static_cast<glm::uint>(2.0f / cellSize);
constexpr glm::uint maxParticles = gridWidth * gridWidth;
glm::uint particlesToCreate = 128;
glm::uint maxParticlesPerCell = 4;

const int workGroupSize = 256; // Must match local_size_x in compute shader
//GLint maxWorkGroups[3];

struct Particle
{
    glm::vec2 position; //[0, 1]
    glm::vec2 velocity; //[2, 3]
    glm::vec3 color;    //[4, 5, 6]
    float size;         //[7]
    float active;       //[8]
    float resting;      //[9]
};

// Quad vertices (x,y)
float quadVertices[] =
{
    -0.5f, -0.5f,
     0.5f, -0.5f,
    -0.5f,  0.5f,
     0.5f,  0.5f
};

int main()
{
    if (!initOpenGL()) return EXIT_FAILURE;

    loadShaders();

    initializeParticleBuffers();

    //calculate max particles
    //glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroups[0]);
    //maxParticles = maxWorkGroups[0] * workGroupSize;

    //initGridLines(cellSize);

    renderLoop();

    cleanup();

    return EXIT_SUCCESS;
}

// Handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        mouseButton = button;

        if (action == GLFW_PRESS)
            isMousePressed = true;
        else if (action == GLFW_RELEASE)
            isMousePressed = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

bool initOpenGL()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "Compute Shader Particle Simulation", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    return true;
}

void loadShaders()
{
    std::string vertexShaderSource = loadShaderFromFile(".\\shaders\\vertex_shader.glsl");
    std::string fragmentShaderSource = loadShaderFromFile(".\\shaders\\fragment_shader.glsl");
    std::string computeShaderSource1 = loadShaderFromFile(".\\shaders\\compute_shader.glsl");
    std::string computeShaderSource2 = loadShaderFromFile(".\\shaders\\compute_shader2.glsl");

    const char* vertexSourceCStr = vertexShaderSource.c_str();
    const char* fragmentSourceCStr = fragmentShaderSource.c_str();
    const char* computeSourceCStr1 = computeShaderSource1.c_str();
    const char* computeSourceCStr2 = computeShaderSource2.c_str();

    shaderProgram = createShaderProgram(vertexSourceCStr, fragmentSourceCStr);
    computeProgramFirst = createComputeShaderProgram(computeSourceCStr1);
    computeProgramSecond = createComputeShaderProgram(computeSourceCStr2);
}

void initializeParticleBuffers()
{
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); // Position attribute (x,y)
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    
    glGenBuffers(2, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &gridSSBO);
    glGenBuffers(1, &gridParticlesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridWidth * gridWidth * sizeof(glm::uint), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridParticlesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridWidth * gridWidth * maxParticlesPerCell * sizeof(glm::uint), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gridSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gridParticlesSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void dispatchFirstCompute(int activeGroups)
{
    float normalizedMouseX = static_cast<float>((mouseX / windowWidth) * 2.0f - 1.0f);
    float normalizedMouseY = static_cast<float>(-((mouseY / windowHeight) * 2.0f - 1.0f));

    glUseProgram(computeProgramFirst);
    // Pass uniforms
    glUniform2f(glGetUniformLocation(computeProgramFirst, "mousePosition"), normalizedMouseX, normalizedMouseY);
    glUniform1i(glGetUniformLocation(computeProgramFirst, "mouseButton"), mouseButton);

    glUniform1i(glGetUniformLocation(computeProgramFirst, "createParticles"), isMousePressed ? 1 : 0);
    glUniform1i(glGetUniformLocation(computeProgramFirst, "particleOffset"), particleOffset - particlesToCreate);

    glUniform1ui(glGetUniformLocation(computeProgramFirst, "particlesToCreate"), particlesToCreate);
    glUniform1ui(glGetUniformLocation(computeProgramFirst, "maxParticles"), maxParticles);
    glUniform1ui(glGetUniformLocation(computeProgramFirst, "maxParticlesPerCell"), maxParticlesPerCell);

    glUniform1ui(glGetUniformLocation(computeProgramFirst, "gridWidth"), gridWidth);
    glUniform1f(glGetUniformLocation(computeProgramFirst, "cellSize"), cellSize);

    glUniform1f(glGetUniformLocation(computeProgramFirst, "gravity"), -0.0001f);
    glUniform1f(glGetUniformLocation(computeProgramFirst, "damping"), 0.975f);
    float currentTime = static_cast<float>(glfwGetTime());
    glUniform1f(glGetUniformLocation(computeProgramFirst, "time"), currentTime);
    glUniform1f(glGetUniformLocation(computeProgramFirst, "spawnRadius"), 0.0001f);

    glDispatchCompute(activeGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure compute writes are visible
}
void dispatchSecondCompute(int activeGroups)
{
    glUseProgram(computeProgramSecond);
    glUniform1ui(glGetUniformLocation(computeProgramSecond, "maxParticlesPerCell"), maxParticlesPerCell);
    glUniform1ui(glGetUniformLocation(computeProgramSecond, "gridWidth"), gridWidth);
    glUniform1f(glGetUniformLocation(computeProgramSecond, "cellSize"), cellSize);

    glDispatchCompute(activeGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure compute writes are visible
}

void renderLoop()
{
    int currentSSBO = 0;
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        if (isMousePressed)
        {
            particleOffset = std::min((particleOffset + particlesToCreate), maxParticles);
        }

        // Clear the gridSSBO and gridParticlesSSBO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridParticlesSSBO);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Use the current SSBO for compute shader updates, use the next SSBO for writing
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[currentSSBO]); //read
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1 - currentSSBO]); //write

        int activeParticles = particleOffset;
        int activeGroups = (activeParticles + workGroupSize - 1) / workGroupSize;
        dispatchFirstCompute(activeGroups);
        dispatchSecondCompute(activeGroups);

        // Bind updated SSBO for rendering, vertex shader uses binding=0
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[1 - currentSSBO]);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render particles as quads
        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, activeParticles);

        currentSSBO = 1 - currentSSBO; // Alternate between 0 and 1

        drawGridLines();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void cleanup()
{
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(2, ssbo);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &gridSSBO);
    glDeleteBuffers(1, &gridParticlesSSBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(computeProgramFirst);

    glfwDestroyWindow(window);
    glfwTerminate();
}
