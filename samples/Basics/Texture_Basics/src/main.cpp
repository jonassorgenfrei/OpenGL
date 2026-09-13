#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
float gMixValue = 0.2f;

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) gMixValue += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) gMixValue -= deltaTime;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) gMixValue = 0.2f;
    gMixValue = std::max(0.0f, std::min(1.0f, gMixValue));
}

GLuint loadTexture(const char* path, GLenum sourceFormat)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 0);
    if (!pixels)
    {
        std::cerr << "Failed to load texture: " << path << '\n';
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, sourceFormat, width, height, 0,
                 sourceFormat, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);
    return texture;
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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Texture Basics", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwTerminate();
        return -1;
    }
    icon(window);

    const float vertices[] = {
        // position   // texture coordinate
        -0.8f, -0.8f, 0.0f, 0.0f,
         0.8f, -0.8f, 1.0f, 0.0f,
         0.8f,  0.8f, 1.0f, 1.0f,
        -0.8f,  0.8f, 0.0f, 1.0f
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

    stbi_set_flip_vertically_on_load(true);
    const GLuint container = loadTexture(FileSystem::getPath("content/images/container.jpg").c_str(), GL_RGB);
    const GLuint face = loadTexture(FileSystem::getPath("content/images/awesomeface.png").c_str(), GL_RGBA);
    if (!container || !face)
    {
        glfwTerminate();
        return -1;
    }

    Shader shader(FileSystem::getSamplePath("shader/texture.vert").c_str(),
                  FileSystem::getSamplePath("shader/texture.frag").c_str());
    shader.use();
    shader.setInt("containerTexture", 0);
    shader.setInt("faceTexture", 1);
    std::cout << "Texture Basics controls:\n"
              << "  Up/Down: blend textures  R: reset  Esc: quit\n";

    double previousTime = glfwGetTime();
    double nextTitleUpdate = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        const double time = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(0.1, time - previousTime));
        previousTime = time;
        processInput(window, deltaTime);

        glClearColor(0.08f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, container);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, face);
        shader.use();
        shader.setFloat("mixValue", gMixValue);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (time >= nextTitleUpdate)
        {
            std::ostringstream title;
            title << "Texture Basics | blend " << std::fixed << std::setprecision(0)
                  << gMixValue * 100.0f << "% | [Up/Down] blend [R] reset [Esc] quit";
            glfwSetWindowTitle(window, title.str().c_str());
            nextTitleUpdate = time + 0.2;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &container);
    glDeleteTextures(1, &face);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwTerminate();
    return 0;
}
