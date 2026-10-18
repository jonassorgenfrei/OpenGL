#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"
#include "modules/help_overlay.h"
#include "virtual_texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <exception>

namespace
{
constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;
constexpr float StartX = 12.5f / 90.0f;
constexpr float StartY = 44.0f / 90.0f;
float gCenterX = StartX;
float gCenterY = StartY;
float gViewSpan = 0.025f;
bool gStreaming = true;
bool gResidency = false;
int gUploadBudget = 4;
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
        gCenterX = StartX;
        gCenterY = StartY;
        gViewSpan = 0.025f;
    }

    if (pressedOnce(window, GLFW_KEY_SPACE)) gStreaming = !gStreaming;
    if (pressedOnce(window, GLFW_KEY_V)) gResidency = !gResidency;
    if (pressedOnce(window, GLFW_KEY_B)) gUploadBudget = gUploadBudget == 4 ? 1 : 4;
    if (pressedOnce(window, GLFW_KEY_O))
    {
        gCenterX = gCenterY = 0.5f;
        gViewSpan = 1.0f;
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
    gViewSpan = clampFloat(gViewSpan, 0.003f, 1.0f);
    const float halfSpan = gViewSpan * 0.5f;
    gCenterX = clampFloat(gCenterX, halfSpan, 1.0f - halfSpan);
    gCenterY = clampFloat(gCenterY, halfSpan, 1.0f - halfSpan);
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Jump to a recognizable region without panning across the whole 16K image.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    int width, height, windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &width, &height);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    if (width <= 0 || height <= 0 || windowWidth <= 0 || windowHeight <= 0) return;
    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    const double x = cursorX * width / windowWidth;
    const double y = height - cursorY * height / windowHeight;
    const int footer = std::min(150, height / 3);
    const int left = width * 2 / 3;
    const int panelHeight = std::max(1, (height - footer - 70) / 2);
    const int panelWidth = std::max(1, width - left - 15);
    const int bottom = height - 45 - panelHeight;
    if (x < left + 5 || x >= left + 5 + panelWidth || y < bottom || y >= bottom + panelHeight) return;
    gCenterX = static_cast<float>((x - left - 5) / panelWidth);
    gCenterY = static_cast<float>((y - bottom) / panelHeight);
}

// Coordinates and row order match the cache preview's OpenGL viewport.
int cacheSlotAt(double x, double y, int width, int height)
{
    if (width <= 0 || height <= 0) return -1;
    const int footer = std::min(150, height / 3);
    const int left = width * 2 / 3;
    const int panelWidth = std::max(1, width - left - 15);
    const int panelHeight = std::max(1, (height - footer - 70) / 2);
    const double u = (x - left - 5) / panelWidth;
    const double v = (y - footer) / panelHeight;
    if (u < 0.0 || u >= 1.0 || v < 0.0 || v >= 1.0) return -1;
    return static_cast<int>(v * VirtualTexture::CachePages) * VirtualTexture::CachePages +
           static_cast<int>(u * VirtualTexture::CachePages);
}

int hoveredCacheSlot(GLFWwindow* window, int width, int height)
{
    if (!glfwGetWindowAttrib(window, GLFW_HOVERED)) return -1;
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) return -1;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    // GLFW cursor positions are window coordinates; the viewports use framebuffer
    // pixels, which may differ on a high-DPI display. Convert and flip Y once.
    return cacheSlotAt(x * width / windowWidth, height - y * height / windowHeight, width, height);
}

void scrollCallback(GLFWwindow*, double, double yOffset)
{
    gPendingScroll += yOffset;
}
}

int runSample()
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
        sample_help::shutdown();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        sample_help::shutdown();
        glfwTerminate();
        return -1;
    }
    icon(window);
    sample_help::initialize(window, {
        {"Esc", "Exit sample"},
        {"WASD / arrows", "Pan texture"},
        {"Mouse wheel", "Zoom texture"},
        {"Q / E", "Zoom in / out"},
        {"R / O", "Reset close-up / whole image"},
        {"V", "Toggle residency colors"},
        {"Click overview", "Jump to another image region"},
        {"Hover cache", "Mark the cached page in the image"},
        {"Space", "Pause / resume streaming"},
        {"C", "Clear detailed pages"},
        {"B", "Upload budget: 1 / 4 pages"}
    });

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
    virtualTexture.initialize(FileSystem::getPath("content/images/virtual_texturing/blue_marble_july_c1.vtpack"));
    Shader shader(FileSystem::getSamplePath("shader/virtual_texture.vert").c_str(),
                  FileSystem::getSamplePath("shader/virtual_texture.frag").c_str());
    shader.use();
    shader.setInt("physicalCache", 0);
    shader.setInt("pageTable", 1);
    shader.setInt("virtualPages", VirtualTexture::VirtualPages);
    shader.setInt("mipLevels", VirtualTexture::MipLevels);
    TextRenderer text;
    const bool textReady = text.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20);
    std::cout << "Virtual texture: 16384x16384 RGBA8 = 1024 MiB base image; bounded cache + table = 2.51 MiB.\n";
    std::cout << "Virtual Texturing controls:\n"
              << "  WASD/arrows: pan  mouse wheel or Q/E: zoom  R: close-up  O: overview\n"
              << "  V: residency colors  Space: pause streaming  C: clear detail  B: upload budget  H: help  Esc: quit\n";

    double previousTime = glfwGetTime();
    double nextTitleUpdate = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        const double time = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(0.1, time - previousTime));
        previousTime = time;
        processInput(window, deltaTime);
        if (pressedOnce(window, GLFW_KEY_C)) virtualTexture.clearDetail();
        virtualTexture.update(gCenterX, gCenterY, gViewSpan, gStreaming, gUploadBudget);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) { glfwPollEvents(); continue; }
        glViewport(0, 0, width, height);

        glClearColor(0.025f, 0.03f, 0.045f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        glUniform2f(glGetUniformLocation(shader.ID, "viewCenter"), gCenterX, gCenterY);
        shader.setFloat("viewSpan", gViewSpan);
        shader.setInt("activeMip", virtualTexture.activeMip());
        const int hoveredSlot = hoveredCacheSlot(window, width, height);
        const auto hoveredPage = virtualTexture.slotMapping(hoveredSlot);
        shader.setInt("hoveredSlot", hoveredSlot);
        shader.setInt("hoveredMip", hoveredPage.mip);
        if (hoveredPage.mip >= 0)
        {
            const float dimension = static_cast<float>(VirtualTexture::VirtualPages >> hoveredPage.mip);
            glUniform4f(glGetUniformLocation(shader.ID, "hoveredBounds"), hoveredPage.x / dimension, hoveredPage.y / dimension,
                             (hoveredPage.x + 1) / dimension, (hoveredPage.y + 1) / dimension);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.cacheTexture());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTableTexture());
        glBindVertexArray(vao);
        // One virtual view, plus the logical image and the bounded backing store.
        const int footer = std::min(150, height / 3);
        const int left = width * 2 / 3;
        const int panelHeight = std::max(1, (height - footer - 70) / 2);
        shader.setInt("displayMode", gResidency ? 1 : 0);
        glViewport(10, footer, std::max(1, left - 20), std::max(1, height - footer - 45));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        shader.setInt("displayMode", 3);
        glViewport(left + 5, height - 45 - panelHeight, std::max(1, width - left - 15), panelHeight);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        shader.setInt("displayMode", 2);
        glViewport(left + 5, footer, std::max(1, width - left - 15), panelHeight);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glViewport(0, 0, width, height);
        if (textReady)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            text.setWindowSize(width, height);
            const float scale = std::min(0.8f, width / 1500.0f);
            auto label = [&](const std::string& line, float x, float y) {
                text.RenderText(line, x, y, scale, {0.9f, 0.94f, 1.0f});
            };
            label(gResidency ? "Virtual view: green = requested detail, orange = coarse fallback" : "NASA Blue Marble: streamed satellite image (V: residency colors)", 115, 17);
            label("Overview: click to jump; yellow = view", static_cast<float>(left + 10), 17);
            label("Physical cache: hover a slot to locate it", static_cast<float>(left + 10), static_cast<float>(height - footer - panelHeight - 23));
            const double cacheMiB = (VirtualTexture::CacheSize * VirtualTexture::CacheSize * 4.0 +
                VirtualTexture::VirtualPages * VirtualTexture::PageTableHeight * 4.0) / (1024.0 * 1024.0);
            std::ostringstream memory, activity;
            memory << "16384 x 16384 RGBA8: 1024 MiB full base image vs " << std::fixed << std::setprecision(2)
                   << cacheMiB << " MiB cache + page table (allocated)";
            activity << (gStreaming ? "STREAMING" : "PAUSED") << " | resident " << virtualTexture.residentCount()
                     << "/121 | pending " << virtualTexture.pendingCount() << " | uploads " << virtualTexture.uploadsThisFrame()
                     << "/" << gUploadBudget << " | evictions " << virtualTexture.evictions() << " | mip " << virtualTexture.activeMip() << " | tile reads " << virtualTexture.diskReads();
            label(memory.str(), 15, static_cast<float>(height - footer + 12));
            label(activity.str(), 15, static_cast<float>(height - footer + 38));
            std::ostringstream hoverInfo;
            if (hoveredSlot >= 0)
            {
                hoverInfo << "Cache slot (" << hoveredSlot % VirtualTexture::CachePages << ", "
                          << hoveredSlot / VirtualTexture::CachePages << ")";
                if (hoveredPage.mip < 0) hoverInfo << " is empty; no image region is mapped.";
                else
                {
                    const int footprint = VirtualTexture::PageSize << hoveredPage.mip;
                    hoverInfo << " -> mip " << hoveredPage.mip << " page (" << hoveredPage.x << ", " << hoveredPage.y
                              << "), " << footprint << " x " << footprint << " image texels | cyan = page footprint";
                }
            }
            else hoverInfo << "Hover the physical cache to mark its page in cyan. Space pauses streaming; C clears detail; B slows uploads.";
            label(hoverInfo.str(), 15, static_cast<float>(height - footer + 64));
            label("WASD/arrows pan | wheel/Q/E zoom | click overview to jump | V colors | O overview | R Italy | H help", 15, static_cast<float>(height - footer + 90));
            label("NASA Blue Marble, July 2004 | 16K image tiles read from disk; full image stays out of GPU and CPU memory.", 15, static_cast<float>(height - footer + 116));
            glDisable(GL_BLEND);
        }

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
        sample_help::render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    text.release();
    virtualTexture.destroy();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    sample_help::shutdown();
    glfwTerminate();
    return 0;
}

int main()
{
    try { return runSample(); }
    catch (const std::exception& error)
    {
        std::cerr << "Virtual texturing: " << error.what() << '\n';
        sample_help::shutdown();
        glfwTerminate();
        return -1;
    }
}
