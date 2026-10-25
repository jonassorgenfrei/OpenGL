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

#include <cmath>
#include <iostream>

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

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
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    sample_help::initialize(window, {{"Esc", "Exit sample"}});
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

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

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

        // NO_WAIT avoids waiting for the query result. If it is not ready yet,
        // OpenGL renders conservatively instead of incorrectly hiding the object.
        shader.setVec3("color", 1.0f, 0.45f, 0.10f);
        glBeginConditionalRender(query, GL_QUERY_NO_WAIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEndConditionalRender();

        // Draw UI after conditional rendering so occlusion never hides help.
        sample_help::render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteQueries(1, &query);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    sample_help::shutdown();
    glfwTerminate();
    return 0;
}
