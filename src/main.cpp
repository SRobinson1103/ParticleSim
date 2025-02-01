#include "ShaderUtil.hpp"

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

GLFWwindow* window;
GLuint shaderProgram, computeProgram;
double mouseX, mouseY;
bool isMousePressed = false;
int mouseButton = 0;

GLuint quadVAO, quadVBO;
GLuint ssbo[2];
GLuint gridSSBO, gridParticlesSSBO;

int particleOffset = 0;
glm::uint particlesToCreate = 10;
glm::uint maxParticles = 4000000;
glm::uint maxParticlesPerCell = 2;
glm::uint gridWidth = 2000;
float cellSize = 0.001;
const int workGroupSize = 256; // Must match local_size_x in compute shader
GLint maxWorkGroups[3];

struct Particle
{
    glm::vec2 position; //[0, 1]
    glm::vec2 velocity; //[2, 3]
    glm::vec3 color;    //[4, 5, 6]
    float size;         //[7]
    float active;       //[8]
};

// Quad vertices (x,y)
float quadVertices[] = {
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
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroups[0]);
    //maxParticles = maxWorkGroups[0] * workGroupSize;

    renderLoop();

    cleanup();

    return EXIT_SUCCESS;
}

// Handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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
    std::string computeShaderSource = loadShaderFromFile(".\\shaders\\compute_shader.glsl");

    const char* vertexSourceCStr = vertexShaderSource.c_str();
    const char* fragmentSourceCStr = fragmentShaderSource.c_str();
    const char* computeSourceCStr = computeShaderSource.c_str();

    shaderProgram = createShaderProgram(vertexSourceCStr, fragmentSourceCStr);
    computeProgram = createComputeShaderProgram(computeSourceCStr);
}

void initializeParticleBuffers()
{
    // Create VBO and VAO for the quad
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute XY
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV attribute for texturing
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

        float normalizedMouseX = (mouseX / 800.0) * 2.0f - 1.0f;
        float normalizedMouseY = -((mouseY / 600.0) * 2.0f - 1.0f);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Use the current SSBO for compute shader updates
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[currentSSBO]); //read
        // use the next SSBO for writing
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1 - currentSSBO]); //write

        glUseProgram(computeProgram);
        // Pass uniforms
        glUniform2f(glGetUniformLocation(computeProgram, "mousePosition"), normalizedMouseX, normalizedMouseY);
        glUniform1i(glGetUniformLocation(computeProgram, "mouseButton"), mouseButton);

        glUniform1i(glGetUniformLocation(computeProgram, "createParticles"), isMousePressed ? 1 : 0);
        glUniform1i(glGetUniformLocation(computeProgram, "particleOffset"), particleOffset - particlesToCreate);

        glUniform1ui(glGetUniformLocation(computeProgram, "particlesToCreate"), particlesToCreate);
        glUniform1ui(glGetUniformLocation(computeProgram, "maxParticles"), maxParticles);
        glUniform1ui(glGetUniformLocation(computeProgram, "maxParticlesPerCell"), maxParticlesPerCell);

        glUniform1ui(glGetUniformLocation(computeProgram, "gridWidth"), gridWidth);
        glUniform1f(glGetUniformLocation(computeProgram, "cellSize"), cellSize);

        glUniform1f(glGetUniformLocation(computeProgram, "gravity"), -0.001f);
        glUniform1f(glGetUniformLocation(computeProgram, "damping"), 0.975f);
        float currentTime = glfwGetTime();
        glUniform1f(glGetUniformLocation(computeProgram, "time"), currentTime);
        glUniform1f(glGetUniformLocation(computeProgram, "spawnRadius"), 0.00001f);

        int activeParticles = particleOffset;
        int activeGroups = (activeParticles + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(activeGroups, 1, 1);

        // Ensure compute writes are visible
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Bind updated SSBO for rendering, vertex shader uses binding=0
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[1 - currentSSBO]);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render particles as points
        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, activeParticles);

        currentSSBO = 1 - currentSSBO; // Alternate between 0 and 1

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
    glDeleteProgram(shaderProgram);
    glDeleteProgram(computeProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
}