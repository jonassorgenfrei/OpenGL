/*
    Batching

    The left half draws 36 compatible quads separately. The right half contains
    the same number of quads in one vertex buffer and draws them as one batch.
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/help_overlay.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// callbacks and input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw: create the window and register callbacks
    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH, SCR_HEIGHT,
        "Batching: left = 36 draw calls, right = 1 draw call",
        nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    sample_help::initialize(window, {{"Esc", "Exit sample"}});
    TextRenderer labels;
    const bool labelsReady = labels.Load(
        FileSystem::getPath("content/fonts/GO-Regular.ttf"), 24);

    Shader shader(
        FileSystem::getSamplePath("shader/batching.vert").c_str(),
        FileSystem::getSamplePath("shader/batching.frag").c_str());

    // One small quad. On the left it is moved and drawn separately 36 times.
    const float quadVertices[] = {
        // position       color
        -0.055f, -0.080f, 1.0f, 0.55f, 0.15f,
         0.055f, -0.080f, 1.0f, 0.55f, 0.15f,
         0.055f,  0.080f, 1.0f, 0.55f, 0.15f,
        -0.055f, -0.080f, 1.0f, 0.55f, 0.15f,
         0.055f,  0.080f, 1.0f, 0.55f, 0.15f,
        -0.055f,  0.080f, 1.0f, 0.55f, 0.15f
    };

    unsigned int unbatchedVAO;
    unsigned int unbatchedVBO;
    glGenVertexArrays(1, &unbatchedVAO);
    glGenBuffers(1, &unbatchedVBO);
    glBindVertexArray(unbatchedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, unbatchedVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // For the batch, append every transformed quad to one CPU-side array.
    // All quads use the same shader and render state, so they are compatible.
    std::vector<float> batchedVertices;
    batchedVertices.reserve(36 * 6 * 5);
    for (int row = 0; row < 6; ++row)
    {
        for (int column = 0; column < 6; ++column)
        {
            const float x = 0.18f + column * 0.14f;
            const float y = -0.75f + row * 0.30f;

            for (int vertex = 0; vertex < 6; ++vertex)
            {
                batchedVertices.push_back(quadVertices[vertex * 5] + x);
                batchedVertices.push_back(quadVertices[vertex * 5 + 1] + y);
                batchedVertices.push_back(0.15f);
                batchedVertices.push_back(0.75f);
                batchedVertices.push_back(1.0f);
            }
        }
    }

    unsigned int batchedVAO;
    unsigned int batchedVBO;
    glGenVertexArrays(1, &batchedVAO);
    glGenBuffers(1, &batchedVBO);
    glBindVertexArray(batchedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, batchedVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 batchedVertices.size() * sizeof(float),
                 batchedVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        unsigned int unbatchedDrawCalls = 0;
        unsigned int batchedDrawCalls = 0;

        // Unbatched: the CPU submits one draw call for every object.
        glBindVertexArray(unbatchedVAO);
        for (int row = 0; row < 6; ++row)
        {
            for (int column = 0; column < 6; ++column)
            {
                shader.setVec2("offset", -0.88f + column * 0.14f,
                              -0.75f + row * 0.30f);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                ++unbatchedDrawCalls;
            }
        }

        // Batched: the CPU submits all compatible geometry in one draw call.
        shader.setVec2("offset", 0.0f, 0.0f);
        glBindVertexArray(batchedVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36 * 6);
        ++batchedDrawCalls;

        // Count geometry submissions only: text and help need additional draws.
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (labelsReady && width > 0 && height > 0)
        {
            labels.setWindowSize(width, height);
            const float scale = std::min(1.0f,
                std::min(width / 1280.0f, height / 720.0f));
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            labels.RenderText(std::to_string(unbatchedDrawCalls) + " draw calls (geometry)",
                              width * 0.06f, height * 0.94f, scale,
                              {1.0f, 0.55f, 0.15f});
            labels.RenderText(std::to_string(batchedDrawCalls) + " draw call (geometry)",
                              width * 0.59f, height * 0.94f, scale,
                              {0.15f, 0.75f, 1.0f});
            glDisable(GL_BLEND);
        }

        sample_help::render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release resources while the OpenGL context is still current
    glDeleteVertexArrays(1, &unbatchedVAO);
    glDeleteBuffers(1, &unbatchedVBO);
    glDeleteVertexArrays(1, &batchedVAO);
    glDeleteBuffers(1, &batchedVBO);
    labels.release();
    sample_help::shutdown();
    glfwTerminate();
    return 0;
}

// Process keyboard input once per frame.
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Match the viewport to the framebuffer dimensions, including HiDPI displays.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
