// A tiny VAT bake and playback example: columns = vertices, rows = frames.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "modules/shader_m.h"
#include "modules/filesystem.h"
#include "modules/help_overlay.h"
#include "modules/window.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
// settings
constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 800;
constexpr int columns = 33;
constexpr int rows = 17;
constexpr int vertexCount = columns * rows;
constexpr int frameCount = 64;
constexpr float framesPerSecond = 24.0f;
constexpr float pi = 3.14159265359f;
bool paused = false;
bool interpolate = true;
bool wireframe = false;
bool showTexture = true;
bool stepFrame = false;
bool reset = false;

// callbacks and helpers
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
std::vector<float> bakePositions();

}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vertex Animation Texture", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);

    icon(window);
    sample_help::initialize(window, {
        {"Esc", "Exit sample"}, {"Space", "Pause / resume animation"},
        {"Right", "Pause and advance one baked frame"},
        {"I", "Toggle interpolation between frames"},
        {"W", "Toggle wireframe"}, {"T", "Show / hide VAT preview"},
        {"R", "Restart animation"}
    });

    // build and compile our shader programs
    // -------------------------------------
    Shader meshShader(FileSystem::getSamplePath("shader/vat.vert").c_str(),
                      FileSystem::getSamplePath("shader/vat.frag").c_str());
    Shader previewShader(FileSystem::getSamplePath("shader/preview.vert").c_str(),
                         FileSystem::getSamplePath("shader/preview.frag").c_str());

    // bake and upload the animation texture once
    // ------------------------------------------
    const auto pixels = bakePositions();
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Signed floating-point RGB stores actual object-space XYZ positions.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, vertexCount, frameCount,
                 0, GL_RGB, GL_FLOAT, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // set up the indexed mesh; positions come from the VAT
    // ----------------------------------------------------
    std::vector<unsigned int> indices;
    for (int row = 0; row < rows - 1; ++row)
    {
        for (int column = 0; column < columns - 1; ++column)
        {
            const unsigned int a = row * columns + column;
            const unsigned int b = a + 1;
            const unsigned int c = a + columns;
            const unsigned int d = c + 1;
            indices.insert(indices.end(), {a, b, c, b, d, c});
        }
    }
    unsigned int vao = 0;
    unsigned int ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
    // No position VBO: indexed gl_VertexID selects the VAT column directly.
    meshShader.use();
    meshShader.setInt("positions", 0);
    previewShader.use();
    previewShader.setInt("positions", 0);

    std::cout << "VAT: " << vertexCount << " vertices x " << frameCount
              << " frames, RGB32F. Press H for controls.\n"
              << "Preview: horizontal = vertex ID, vertical = frame; white = playback row.\n";
    // timing
    // ------
    double previousTime = glfwGetTime();
    float currentFrame = 0.0f;
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        glfwPollEvents();
        processInput(window);

        // update animation playback
        // -------------------------
        const double now = glfwGetTime();
        const float elapsed = static_cast<float>(now - previousTime);
        previousTime = now;
        if (!paused)
            currentFrame += elapsed * framesPerSecond;
        if (reset)
        {
            currentFrame = 0.0f;
            reset = false;
        }
        if (stepFrame)
        {
            currentFrame = std::floor(currentFrame) + 1.0f;
            stepFrame = false;
        }
        currentFrame = std::fmod(currentFrame, float(frameCount));

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0)
        {
            glfwWaitEventsTimeout(0.05);
            continue;
        }

        // render
        // ------
        glViewport(0, 0, width, height);
        glClearColor(0.055f, 0.075f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        meshShader.use();
        meshShader.setFloat("currentFrame", currentFrame);
        meshShader.setBool("interpolateFrames", interpolate);
        const glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 1.7f, 6.5f),
                                          glm::vec3(0.0f), glm::vec3(0, 1, 0));
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                     float(width) / height, 0.1f, 50.0f);
        meshShader.setMat4("viewProjection", projection * view);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

        // VAT preview
        // -----------
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (showTexture)
        {
            glDisable(GL_DEPTH_TEST);
            glViewport(width * 2 / 3, height * 3 / 4, width / 3, height / 4);
            previewShader.use();
            previewShader.setFloat("currentFrame", currentFrame);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glViewport(0, 0, width, height);
        // help overlay and swap buffers
        // -----------------------------
        sample_help::render();
        glfwSwapBuffers(window);
    }

    // release resources before destroying the context
    // -----------------------------------------------
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(meshShader.ID);
    glDeleteProgram(previewShader.ID);
    sample_help::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

namespace
{
// glfw: update the viewport when the framebuffer size changes
void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Toggle actions run only on key presses, not on key-repeat events.
void key_callback(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS)
        return;
    switch (key)
    {
    case GLFW_KEY_SPACE: paused = !paused; break;
    case GLFW_KEY_I: interpolate = !interpolate; break;
    case GLFW_KEY_W: wireframe = !wireframe; break;
    case GLFW_KEY_T: showTexture = !showTexture; break;
    case GLFW_KEY_RIGHT: paused = true; stepFrame = true; break;
    case GLFW_KEY_R: reset = true; break;
    }
}

std::vector<float> bakePositions()
{
    // Bake once on the CPU. Playback only samples this texture; it never
    // evaluates the wave again or uploads new vertex positions.
    std::vector<float> pixels(vertexCount * frameCount * 3);
    for (int frame = 0; frame < frameCount; ++frame)
    {
        // Exclude the duplicate endpoint so the last frame wraps to frame 0.
        const float phase = 2.0f * pi * frame / frameCount;
        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                const float u = float(column) / (columns - 1);
                const float v = float(row) / (rows - 1);
                const int vertex = row * columns + column;
                const int texel = (frame * vertexCount + vertex) * 3;
                pixels[texel] = (u - 0.5f) * 4.0f;
                pixels[texel + 1] = (v - 0.5f) * 2.4f;
                // Left edge stays fixed, the free edge waves in depth.
                pixels[texel + 2] = u * (0.45f * std::sin(4.0f * pi * u - phase)
                    + 0.15f * std::sin(2.0f * pi * v - phase));
            }
        }
    }
    return pixels;
}
} // namespace
