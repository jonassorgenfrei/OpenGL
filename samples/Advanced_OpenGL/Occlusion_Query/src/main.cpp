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
#include <vector>

// callbacks and input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderStatus(GLFWwindow* window, TextRenderer& text, bool rendered);
std::vector<glm::vec2> createGearVertices();

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
bool showBoundingBox = false;

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

    sample_help::initialize(window, {
        {"Esc", "Exit sample"},
        {"B", "Show / hide bounding box"}
    });
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
        -0.5f,  0.5f,
        // Separate outline for visualization; the query uses the filled triangles.
        -0.5f, -0.5f,
         0.5f, -0.5f,
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

    // The real mesh has 192 triangles; its axis-aligned bound needs only two.
    const std::vector<glm::vec2> gearVertices = createGearVertices();
    glm::vec2 boundsMin = gearVertices.front();
    glm::vec2 boundsMax = gearVertices.front();
    for (const glm::vec2& vertex : gearVertices)
    {
        boundsMin = glm::min(boundsMin, vertex);
        boundsMax = glm::max(boundsMax, vertex);
    }
    const glm::vec2 boundsCenter = (boundsMin + boundsMax) * 0.5f;
    const glm::vec2 boundsSize = boundsMax - boundsMin;
    const glm::vec2 objectScale(0.44f, 0.60f);

    unsigned int objectVAO, objectVBO;
    glGenVertexArrays(1, &objectVAO);
    glGenBuffers(1, &objectVBO);
    glBindVertexArray(objectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, objectVBO);
    glBufferData(GL_ARRAY_BUFFER, gearVertices.size() * sizeof(glm::vec2),
                 gearVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
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

        const float objectX = std::sin(static_cast<float>(glfwGetTime())) * 0.72f;
        const glm::vec2 objectOffset(objectX, 0.0f);
        const glm::vec2 boxOffset = objectOffset + boundsCenter * objectScale;
        const glm::vec2 boxScale = boundsSize * objectScale;
        shader.setVec2("offset", boxOffset);
        shader.setVec2("scale", boxScale);
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
        shader.setVec2("offset", objectOffset);
        shader.setVec2("scale", objectScale);
        glBindVertexArray(objectVAO);
        glBeginConditionalRender(query, GL_QUERY_WAIT);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(gearVertices.size()));
        glEndConditionalRender();

        // Debug outline is deliberately visible through the occluder. Drawing it
        // after the query with depth writes disabled cannot affect the result.
        if (showBoundingBox)
        {
            shader.setVec2("offset", boxOffset);
            shader.setVec2("scale", boxScale);
            shader.setVec3("color", 1.0f, 0.9f, 0.2f);
            glBindVertexArray(VAO);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDrawArrays(GL_LINE_LOOP, 6, 4);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        }

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
    glDeleteVertexArrays(1, &objectVAO);
    glDeleteBuffers(1, &objectVBO);
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

    // Toggle once per key press, rather than once per frame while held down.
    static bool bWasDown = false;
    const bool bDown = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
    if (bDown && !bWasDown)
        showBoundingBox = !showBoundingBox;
    bWasDown = bDown;
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
    text.RenderText(rendered ? "Rendered - potentially visible"
                             : "Not rendered - occluded",
                    width * 0.06f, height * 0.90f, scale,
                    rendered ? glm::vec3(0.3f, 0.9f, 0.5f)
                             : glm::vec3(1.0f, 0.45f, 0.3f));
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// A toothed ring with an open center makes the difference from its box visible.
std::vector<glm::vec2> createGearVertices()
{
    constexpr int segments = 96;
    constexpr float twoPi = 6.28318530718f;
    std::vector<glm::vec2> vertices;
    vertices.reserve(segments * 6);
    const auto outerRadius = [](int index)
    {
        const int toothStep = index % 8;
        return toothStep >= 2 && toothStep <= 5 ? 0.5f : 0.4f;
    };
    for (int i = 0; i < segments; ++i)
    {
        const float a = twoPi * i / segments;
        const float b = twoPi * (i + 1) / segments;
        const glm::vec2 directionA(std::cos(a), std::sin(a));
        const glm::vec2 directionB(std::cos(b), std::sin(b));
        const glm::vec2 innerA = directionA * 0.18f;
        const glm::vec2 innerB = directionB * 0.18f;
        const glm::vec2 outerA = directionA * outerRadius(i);
        const glm::vec2 outerB = directionB * outerRadius((i + 1) % segments);
        vertices.insert(vertices.end(), {innerA, outerA, outerB,
                                         innerA, outerB, innerB});
    }
    return vertices;
}
