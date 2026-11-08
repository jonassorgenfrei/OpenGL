/*
    Procedural noise comparison

    Compares interpolated value noise, Perlin gradient noise, layered Perlin
    noise (fBm), and temporally coherent animated 3D Perlin noise. All variants
    are evaluated in the fragment shader without reading a noise texture.
    Use 1-4 to select a variant, Up/Down to change its spatial scale, and Space
    to pause the animated mode.
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/help_overlay.h"
#include "modules/sample_input.h"
#include "modules/shader_m.h"
#include "modules/window.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr unsigned int SCR_WIDTH = 1000;
constexpr unsigned int SCR_HEIGHT = 700;

int gNoiseType = 1;
float gScale = 7.0f;
float gAnimationTime = 0.0f;
bool gPaused = false;

const char *noiseName()
{
    constexpr const char *names[] = {"Value", "Perlin", "Fractal Perlin (fBm)", "Animated 3D Perlin"};
    return names[gNoiseType];
}

void framebufferSizeCallback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(float deltaTime)
{
    for (int index = 0; index < 4; ++index)
    {
        if (sample_input::pressed[GLFW_KEY_1 + index])
            gNoiseType = index;
    }

    if (sample_input::pressed[GLFW_KEY_SPACE])
        gPaused = !gPaused;
    if (sample_input::down[GLFW_KEY_UP] || sample_input::down[GLFW_KEY_EQUAL])
        gScale += 5.0f * deltaTime;
    if (sample_input::down[GLFW_KEY_DOWN] || sample_input::down[GLFW_KEY_MINUS])
        gScale -= 5.0f * deltaTime;
    if (sample_input::pressed[GLFW_KEY_R])
    {
        gNoiseType = 1;
        gScale = 7.0f;
        gAnimationTime = 0.0f;
        gPaused = false;
    }
    gScale = std::clamp(gScale, 1.0f, 30.0f);
}
} // namespace

int main()
{
    GLFWwindow *window = nullptr;
    GLuint vertexArray = 0;
    GLuint shaderProgram = 0;

    try
    {
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Noise Types", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create GLFW window");
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Failed to initialize GLAD");

        glfwSwapInterval(1);
        icon(window);
        sample_help::initialize(window,
                                {{"1", "Value noise"},
                                 {"2", "Perlin gradient noise"},
                                 {"3", "Fractal Perlin (fBm)"},
                                 {"4", "Animated 3D Perlin"},
                                 {"Up / Down", "Change spatial scale"},
                                 {"Space", "Pause / resume animation"},
                                 {"R", "Reset sample"},
                                 {"Esc", "Exit sample"}});

        Shader shader(FileSystem::getSamplePath("shader/noise.vert").c_str(),
                      FileSystem::getSamplePath("shader/noise.frag").c_str());
        shaderProgram = shader.ID;
        GLint linked = GL_FALSE;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
        if (!linked)
            throw std::runtime_error("Noise shaders could not be linked; check the sidecar files");

        // The vertex shader builds a full-screen triangle from gl_VertexID.
        glGenVertexArrays(1, &vertexArray);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        framebufferSizeCallback(window, width, height);

        double previousTime = glfwGetTime();
        double nextTitleUpdate = 0.0;
        while (!glfwWindowShouldClose(window))
        {
            sample_input::poll(window);
            const double currentTime = glfwGetTime();
            const float deltaTime = static_cast<float>(std::min(0.1, currentTime - previousTime));
            previousTime = currentTime;
            processInput(deltaTime);
            if (!gPaused)
                gAnimationTime += deltaTime;

            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwWaitEventsTimeout(0.05);
                continue;
            }

            glClearColor(0.04f, 0.05f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();
            shader.setInt("noiseType", gNoiseType);
            shader.setFloat("scale", gScale);
            shader.setFloat("time", gAnimationTime);
            glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), static_cast<float>(width),
                        static_cast<float>(height));
            glBindVertexArray(vertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            if (currentTime >= nextTitleUpdate)
            {
                std::ostringstream title;
                title << "Noise Types | " << noiseName() << " | scale " << std::fixed << std::setprecision(1)
                      << gScale;
                if (gPaused)
                    title << " | paused";
                glfwSetWindowTitle(window, title.str().c_str());
                nextTitleUpdate = currentTime + 0.2;
            }

            sample_help::render();
            glfwSwapBuffers(window);
        }

        glDeleteVertexArrays(1, &vertexArray);
        glDeleteProgram(shaderProgram);
        sample_help::shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Noise sample: " << error.what() << '\n';
        if (window)
        {
            if (vertexArray)
                glDeleteVertexArrays(1, &vertexArray);
            if (shaderProgram)
                glDeleteProgram(shaderProgram);
            sample_help::shutdown();
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return -1;
    }
}
