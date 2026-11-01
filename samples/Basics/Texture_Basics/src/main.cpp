#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/window.h"
#include "modules/sample_input.h"
#include "modules/help_overlay.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace
{
// settings
constexpr unsigned int SCR_WIDTH = 800;
constexpr unsigned int SCR_HEIGHT = 600;

float gMixValue = 0.2f;

// Input, callbacks and rendering helpers are defined after main().
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(float deltaTime);
GLuint loadTexture(const char *path);
} // namespace

int main()
{
    try
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
        GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Texture Basics", nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
        int framebufferWidth = 0, framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        framebuffer_size_callback(window, framebufferWidth, framebufferHeight);
        icon(window);
        sample_help::initialize(window,
                                {{"Esc", "Exit sample"}, {"Up / Down", "Adjust texture mix"}, {"R", "Reset texture mix"}});

        // set up vertex data and configure vertex attributes
        // --------------------------------------------------
        const float vertices[] = {// position   // texture coordinate
                                  -0.8f, -0.8f, 0.0f, 0.0f, 0.8f,  -0.8f, 1.0f, 0.0f,
                                  0.8f,  0.8f,  1.0f, 1.0f, -0.8f, 0.8f,  0.0f, 1.0f};
        const unsigned int indices[] = {0, 1, 2, 0, 2, 3};
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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        stbi_set_flip_vertically_on_load(true);
        const GLuint container = loadTexture(FileSystem::getPath("content/images/container.jpg").c_str());
        const GLuint face = loadTexture(FileSystem::getPath("content/images/awesomeface.png").c_str());
        if (!container || !face)
        {
            sample_help::shutdown();
            glfwTerminate();
            return -1;
        }

        // build and compile our shader program
        // ------------------------------------
        Shader shader(FileSystem::getSamplePath("shader/texture.vert").c_str(),
                      FileSystem::getSamplePath("shader/texture.frag").c_str());
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
        if (!linked)
            throw std::runtime_error("Texture sample shader could not be linked; check the sidecar files");
        shader.use();
        shader.setInt("containerTexture", 0);
        shader.setInt("faceTexture", 1);

        double previousTime = glfwGetTime();
        double nextTitleUpdate = 0.0;
        // render loop
        // -----------
        while (!glfwWindowShouldClose(window))
        {
            // input
            // -----
            sample_input::poll(window);
            const double time = glfwGetTime();
            const float deltaTime = static_cast<float>(std::min(0.1, time - previousTime));
            previousTime = time;
            processInput(deltaTime);

            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwWaitEventsTimeout(0.05);
                continue;
            }

            // render
            // ------
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
                title << "Texture Basics | blend " << std::fixed << std::setprecision(0) << gMixValue * 100.0f
                      << "%";
                glfwSetWindowTitle(window, title.str().c_str());
                nextTitleUpdate = time + 0.2;
            }
            // help overlay and swap buffers
            // -----------------------------
            sample_help::render();
            glfwSwapBuffers(window);
        }

        glDeleteTextures(1, &container);
        glDeleteTextures(1, &face);
        // release geometry resources
        // --------------------------
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteProgram(shader.ID);
        sample_help::shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Texture sample: " << error.what() << '\n';
        sample_help::shutdown();
        glfwTerminate();
        return -1;
    }
}

namespace
{
// glfw: update the viewport when the framebuffer size changes
void framebuffer_size_callback(GLFWwindow *, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(float deltaTime)
{
    if (sample_input::down[GLFW_KEY_UP])
        gMixValue += deltaTime;
    if (sample_input::down[GLFW_KEY_DOWN])
        gMixValue -= deltaTime;
    if (sample_input::down[GLFW_KEY_R])
        gMixValue = 0.2f;
    gMixValue = std::max(0.0f, std::min(1.0f, gMixValue));
}

GLuint loadTexture(const char *path)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 0);
    if (!pixels)
    {
        std::cerr << "Failed to load texture: " << path << '\n';
        return 0;
    }

    const GLenum formats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    const GLenum internalFormats[] = {GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};
    const GLenum sourceFormat = formats[channels - 1];
    GLint unpackAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // RGB rows need not be a multiple of four bytes.
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormats[channels - 1], width, height, 0, sourceFormat, GL_UNSIGNED_BYTE,
                 pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
    stbi_image_free(pixels);
    return texture;
}
} // namespace
