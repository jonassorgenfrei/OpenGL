#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"
#include "virtual_texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 700;
float gCenterX = 0.5f;
float gCenterY = 0.5f;
float gViewSpan = 0.2f;
double gPendingScroll = 0.0;
std::array<bool, GLFW_KEY_LAST + 1> gPreviousKeys{};

float clampFloat(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

bool pressedOnce(GLFWwindow* window, int key)
{
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool result = pressed && !gPreviousKeys[key];
    gPreviousKeys[key] = pressed;
    return result;
}

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (pressedOnce(window, GLFW_KEY_R))
    {
        gCenterX = gCenterY = 0.5f;
        gViewSpan = 0.2f;
    }

    const float movement = gViewSpan * deltaTime * 0.8f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) gCenterX -= movement;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) gCenterX += movement;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) gCenterY -= movement;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) gCenterY += movement;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) gViewSpan *= std::pow(0.35f, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) gViewSpan *= std::pow(2.85f, deltaTime);
    if (gPendingScroll != 0.0)
    {
        gViewSpan *= std::pow(0.82f, static_cast<float>(gPendingScroll));
        gPendingScroll = 0.0;
    }
    gViewSpan = clampFloat(gViewSpan, 0.025f, 1.0f);
    const float halfSpan = gViewSpan * 0.5f;
    gCenterX = clampFloat(gCenterX, halfSpan, 1.0f - halfSpan);
    gCenterY = clampFloat(gCenterY, halfSpan, 1.0f - halfSpan);
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void scrollCallback(GLFWwindow*, double, double yOffset)
{
    gPendingScroll += yOffset;
}
}

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "Virtual Texturing", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwTerminate();
        return -1;
    }
    icon(window);

    const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f
    };
    const unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    VirtualTexture virtualTexture;
    virtualTexture.initialize();
    Shader shader(FileSystem::getSamplePath("shader/virtual_texture.vert").c_str(),
                  FileSystem::getSamplePath("shader/virtual_texture.frag").c_str());
    shader.use();
    shader.setInt("physicalCache", 0);
    shader.setInt("pageTable", 1);
    std::cout << "Virtual Texturing controls:\n"
              << "  WASD/arrows: pan  mouse wheel or Q/E: zoom  R: reset  Esc: quit\n";

    double previousTime = glfwGetTime();
    double nextTitleUpdate = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        const double time = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(0.1, time - previousTime));
        previousTime = time;
        processInput(window, deltaTime);
        virtualTexture.update(gCenterX, gCenterY, gViewSpan);

        glClearColor(0.025f, 0.03f, 0.045f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        glUniform2f(glGetUniformLocation(shader.ID, "viewCenter"), gCenterX, gCenterY);
        shader.setFloat("viewSpan", gViewSpan);
        shader.setInt("activeMip", virtualTexture.activeMip());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.cacheTexture());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTableTexture());
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (time >= nextTitleUpdate)
        {
            std::ostringstream title;
            title << "Virtual Texturing | cache " << virtualTexture.residentCount()
                  << "/" << virtualTexture.slotCount()
                  << ", uploads " << virtualTexture.uploadsThisFrame() << "/frame"
                  << ", mip " << virtualTexture.activeMip()
                  << ", view " << std::fixed << std::setprecision(1) << gViewSpan * 100.0f
                  << "% | [WASD/arrows] pan [wheel/Q/E] zoom [R] reset [Esc] quit";
            glfwSetWindowTitle(window, title.str().c_str());
            nextTitleUpdate = time + 0.2;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    virtualTexture.destroy();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwTerminate();
    return 0;
}
