#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "compression_encoders.h"
#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr int WINDOW_WIDTH = 900;
constexpr int WINDOW_HEIGHT = 700;
constexpr int IMAGE_SIZE = 256;
enum class Mode { Source, Dxt1, Astc };

Mode gMode = Mode::Source;
std::array<bool, GLFW_KEY_LAST + 1> gPreviousKeys{};

bool pressedOnce(GLFWwindow* window, int key)
{
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool result = pressed && !gPreviousKeys[key];
    gPreviousKeys[key] = pressed;
    return result;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (pressedOnce(window, GLFW_KEY_1)) gMode = Mode::Source;
    if (pressedOnce(window, GLFW_KEY_2)) gMode = Mode::Dxt1;
    if (pressedOnce(window, GLFW_KEY_3)) gMode = Mode::Astc;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLuint createRgbaTexture(const std::vector<std::uint8_t>& pixels)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMAGE_SIZE, IMAGE_SIZE, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}

GLuint createCompressedTexture(GLenum format, const std::vector<std::uint8_t>& blocks)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Extension presence alone is insufficient: confirm that storage exists
    // after the driver processes the compressed upload.
    while (glGetError() != GL_NO_ERROR) {}
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, IMAGE_SIZE, IMAGE_SIZE, 0,
                           static_cast<GLsizei>(blocks.size()), blocks.data());
    GLint compressed = GL_FALSE;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &compressed);
    if (glGetError() == GL_NO_ERROR && compressed == GL_TRUE) return texture;
    glDeleteTextures(1, &texture);
    return 0;
}

std::string modeName(bool dxtSupported, bool astcSupported)
{
    if (gMode == Mode::Source) return "RGBA8 source (256 KiB)";
    if (gMode == Mode::Dxt1)
        return dxtSupported ? "DXT1 / BC1 (32 KiB, 8:1)" : "DXT1 unavailable - RGBA8 fallback";
    return astcSupported ? "ASTC 4x4 (64 KiB, 4:1)" : "ASTC unavailable - RGBA8 fallback";
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
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Texture Compression", nullptr, nullptr);
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

    const bool dxtExtension = GLAD_GL_EXT_texture_compression_s3tc != 0;
    const bool astcExtension = GLAD_GL_KHR_texture_compression_astc_ldr != 0;
    const auto sourcePixels = compression::makeTestImage(IMAGE_SIZE);
    const auto dxtBlocks = compression::encodeDxt1(sourcePixels, IMAGE_SIZE, IMAGE_SIZE);
    const auto astcBlocks = compression::encodeAstc4x4(sourcePixels, IMAGE_SIZE, IMAGE_SIZE);
    const GLuint sourceTexture = createRgbaTexture(sourcePixels);
    GLuint dxtTexture = dxtExtension ? createCompressedTexture(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxtBlocks) : 0;
    GLuint astcTexture = astcExtension ? createCompressedTexture(GL_COMPRESSED_RGBA_ASTC_4x4_KHR, astcBlocks) : 0;
    const bool dxtSupported = dxtTexture != 0;
    const bool astcSupported = astcTexture != 0;
    if (!dxtTexture) dxtTexture = sourceTexture;
    if (!astcTexture) astcTexture = sourceTexture;

    const char* dxtStatus = dxtSupported ? "yes" :
        (dxtExtension ? "no (upload rejected; using RGBA8 fallback)" : "no (extension unavailable; using RGBA8 fallback)");
    const char* astcStatus = astcSupported ? "yes" :
        (astcExtension ? "no (upload rejected; using RGBA8 fallback)" : "no (extension unavailable; using RGBA8 fallback)");
    std::cout << "Texture Compression controls:\n  1: RGBA8  2: DXT1/BC1  3: ASTC 4x4  Esc: quit\n\n"
              << "OpenGL renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
              << "GPU DXT/S3TC support: " << dxtStatus << '\n'
              << "GPU ASTC LDR support: " << astcStatus << '\n'
              << "Payloads: RGBA8=" << sourcePixels.size() << ", DXT1=" << dxtBlocks.size()
              << ", ASTC=" << astcBlocks.size() << " bytes\n";

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

    Shader shader(FileSystem::getSamplePath("shader/compression.vert").c_str(),
                  FileSystem::getSamplePath("shader/compression.frag").c_str());
    shader.use();
    shader.setInt("displayTexture", 0);
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        GLuint displayed = sourceTexture;
        if (gMode == Mode::Dxt1) displayed = dxtTexture;
        if (gMode == Mode::Astc) displayed = astcTexture;
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, displayed);
        shader.use();
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        const std::string title = "Texture Compression | " + modeName(dxtSupported, astcSupported) +
                                  " | [1-3] mode [Esc] quit";
        glfwSetWindowTitle(window, title.c_str());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (dxtSupported) glDeleteTextures(1, &dxtTexture);
    if (astcSupported) glDeleteTextures(1, &astcTexture);
    glDeleteTextures(1, &sourceTexture);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwTerminate();
    return 0;
}
