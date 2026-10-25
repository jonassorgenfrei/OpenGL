/*
    Occlusion query

    A moving orange object passes behind a gray occluder. A query first tests
    its bounding rectangle. OpenGL only executes the real draw when at least
    one sample of that rectangle is visible.
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/help_overlay.h"
#include "modules/font.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// callbacks and input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderStatus(GLFWwindow* window, TextRenderer& text, bool rendered);

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
        SCR_WIDTH, SCR_HEIGHT, "Occlusion query: moving object behind occluder",
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
    TextRenderer statusText;
    const bool statusTextReady = statusText.Load(
        FileSystem::getPath("content/fonts/GO-Regular.ttf"), 24);
    glEnable(GL_DEPTH_TEST);

    Shader shader(
        FileSystem::getSamplePath("shader/occlusion_query.vert").c_str(),
        FileSystem::getSamplePath("shader/occlusion_query.frag").c_str());

    const float rectangleVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    unsigned int VAO;
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices),
                 rectangleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    unsigned int query;
    glGenQueries(1, &query);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);

        // Draw the nearer occluder first so it fills the depth buffer.
        shader.setVec2("offset", 0.0f, 0.0f);
        shader.setVec2("scale", 0.65f, 1.40f);
        shader.setFloat("depth", 0.0f);
        shader.setVec3("color", 0.35f, 0.40f, 0.48f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        const float objectX = std::sin(static_cast<float>(glfwGetTime())) * 0.85f;
        shader.setVec2("offset", objectX, 0.0f);
        shader.setVec2("scale", 0.22f, 0.22f);
        shader.setFloat("depth", 0.3f); // farther away than the occluder

        // Test a cheap conservative bound. It must affect neither color nor depth.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEndQuery(GL_ANY_SAMPLES_PASSED);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // WAIT makes the current frame's render/skip decision deterministic.
        // The status readback below deliberately synchronizes this teaching demo.
        shader.setVec3("color", 1.0f, 0.45f, 0.10f);
        glBeginConditionalRender(query, GL_QUERY_WAIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEndConditionalRender();

        GLuint visible = GL_FALSE;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &visible);
        if (statusTextReady)
            renderStatus(window, statusText, visible != GL_FALSE);

        // Draw UI after conditional rendering so occlusion never hides help.
        sample_help::render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release resources while the OpenGL context is still current
    glDeleteQueries(1, &query);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    statusText.release();
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

// Keep the status below the scene and outside the conditional draw.
void renderStatus(GLFWwindow* window, TextRenderer& text, bool rendered)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0)
        return;

    text.setWindowSize(width, height);
    const float scale = std::min(1.0f,
        std::min(width / 1280.0f, height / 720.0f));
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    text.RenderText(rendered ? "Rendered - object visible"
                             : "Not rendered - occluded",
                    width * 0.06f, height * 0.90f, scale,
                    rendered ? glm::vec3(0.3f, 0.9f, 0.5f)
                             : glm::vec3(1.0f, 0.45f, 0.3f));
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
