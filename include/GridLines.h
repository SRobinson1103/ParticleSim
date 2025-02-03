#ifndef GRIDLINES_H
#define GRIDLINES_H

#include "ShaderUtil.hpp"

#include <string>
#include <vector>
#include <glm/glm.hpp>

float gridCellSize = 0.01f;
std::vector<glm::vec2> gridVertices;
GLuint gridShaderProgram;
GLuint gridVAO, gridVBO;
bool linesIinitialized = false;

void initGridLines(float cellSize)
{
    linesIinitialized = true;
    gridCellSize = cellSize;
    // Create vertical lines
    for (float x = -1.0f; x <= 1.0f; x += gridCellSize) {
        gridVertices.push_back(glm::vec2(x, -1.0f));  // start of line
        gridVertices.push_back(glm::vec2(x, 1.0f));  // end of line
    }

    // Create horizontal lines
    for (float y = -1.0f; y <= 1.0f; y += gridCellSize) {
        gridVertices.push_back(glm::vec2(-1.0f, y));  // start of line
        gridVertices.push_back(glm::vec2(1.0f, y));   // end of line
    }

    std::string vertexShaderSource = loadShaderFromFile(".\\shaders\\vert_shader_line.glsl");
    std::string fragmentShaderSource = loadShaderFromFile(".\\shaders\\frag_shader_line.glsl");

    const char* vertexSourceCStr = vertexShaderSource.c_str();
    const char* fragmentSourceCStr = fragmentShaderSource.c_str();

    gridShaderProgram = createShaderProgram(vertexSourceCStr, fragmentSourceCStr);

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * gridVertices.size(), gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawGridLines()
{
    if (!linesIinitialized)
        return;

    glUseProgram(gridShaderProgram);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertices.size());
}

#endif