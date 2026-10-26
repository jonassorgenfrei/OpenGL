#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/window.h"
#include "modules/sample_input.h"
#include "modules/help_overlay.h"
#include "virtual_texture.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <exception>

namespace
{
// settings
constexpr unsigned int SCR_WIDTH = 1200;
constexpr unsigned int SCR_HEIGHT = 800;

constexpr float StartX = 12.5f / 90.0f;
constexpr float StartY = 44.0f / 90.0f;
float gCenterX = StartX;
float gCenterY = StartY;
float gViewSpan = 0.025f;
bool gStreaming = true;
bool gResidency = false;
int gUploadBudget = 4;
double gPendingScroll = 0.0;

// Input, callbacks and rendering helpers are defined after main().
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(float deltaTime);
void scroll_callback(GLFWwindow *, double, double yOffset);
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
        GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Virtual Texturing", nullptr, nullptr);
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
        glfwSetScrollCallback(window, scroll_callback);
        sample_help::initialize(window, {{"Esc", "Exit sample"},
                                         {"WASD / arrows", "Pan texture"},
                                         {"Mouse wheel", "Zoom texture"},
                                         {"Q / E", "Zoom in / out"},
                                         {"R / O", "Reset close-up / whole image"},
                                         {"V", "Toggle residency colors"},
                                         {"Click overview", "Jump to another image region"},
                                         {"Hover cache", "Mark the cached page in the image"},
                                         {"Space", "Pause / resume streaming"},
                                         {"C", "Clear detailed pages"},
                                         {"B", "Upload budget: 1 / 4 pages"}});

        // set up vertex data and configure vertex attributes
        // --------------------------------------------------
        const float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                                  1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};
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

        virtual_texture::State virtualTexture;
        virtual_texture::initialize(virtualTexture,
                                    FileSystem::getPath("content/images/virtual_texturing/blue_marble_july_c1.vtpack"));
        // build and compile our shader program
        // ------------------------------------
        Shader shader(FileSystem::getSamplePath("shader/virtual_texture.vert").c_str(),
                      FileSystem::getSamplePath("shader/virtual_texture.frag").c_str());
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
        if (!linked)
            throw std::runtime_error("Texture sample shader could not be linked; check the sidecar files");
        shader.use();
        shader.setInt("physicalCache", 0);
        shader.setInt("pageTable", 1);
        shader.setInt("virtualPages", virtual_texture::VirtualPages);
        shader.setInt("mipLevels", virtual_texture::MipLevels);
        shader.setFloat("pageSize", static_cast<float>(virtual_texture::PageSize));
        shader.setFloat("slotSize", static_cast<float>(virtual_texture::SlotSize));
        shader.setFloat("cacheSize", static_cast<float>(virtual_texture::CacheSize));
        TextRenderer text;
        const bool textReady = text.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20);
        const double fullMiB = 4.0 * virtual_texture::VirtualSize * virtual_texture::VirtualSize / (1024.0 * 1024.0);
        const double cacheMiB = (4.0 * virtual_texture::CacheSize * virtual_texture::CacheSize +
                                 4.0 * virtual_texture::VirtualPages * virtual_texture::PageTableHeight) /
                                (1024.0 * 1024.0);
        std::cout << "Virtual texture: " << virtual_texture::VirtualSize << " x " << virtual_texture::VirtualSize
                  << " RGBA8 = " << fullMiB << " MiB base image; bounded cache + table = " << std::fixed
                  << std::setprecision(2) << cacheMiB << " MiB.\n";
        std::cout
            << "Virtual Texturing controls:\n"
            << "  WASD/arrows: pan  mouse wheel or Q/E: zoom  R: close-up  O: overview\n"
            << "  V: residency colors  Space: pause streaming  C: clear detail  B: upload budget  H: help  Esc: quit\n";

        double previousTime = glfwGetTime();
        double nextTitleUpdate = 0.0;
        bool mouseWasDown = false;
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
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwWaitEventsTimeout(0.05);
                continue;
            }
            // Render and hit-test the same panel rectangles, in framebuffer pixels.
            const int footer = std::min(150, height / 3);
            const int left = width * 2 / 3;
            const int panelWidth = std::max(1, width - left - 15);
            const int panelHeight = std::max(1, (height - footer - 70) / 2);
            const int overviewBottom = height - 45 - panelHeight;
            int hoveredSlot = -1;
            const bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            int windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            if (glfwGetWindowAttrib(window, GLFW_HOVERED) && windowWidth > 0 && windowHeight > 0)
            {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                x *= static_cast<double>(width) / windowWidth;
                y = height - y * height / windowHeight;
                const double u = (x - left - 5) / panelWidth;
                const double cacheV = (y - footer) / panelHeight;
                const double overviewV = (y - overviewBottom) / panelHeight;
                if (u >= 0.0 && u < 1.0)
                {
                    if (cacheV >= 0.0 && cacheV < 1.0)
                        hoveredSlot = static_cast<int>(cacheV * virtual_texture::CachePages) * virtual_texture::CachePages +
                                      static_cast<int>(u * virtual_texture::CachePages);
                    if (mouseDown && !mouseWasDown && overviewV >= 0.0 && overviewV < 1.0)
                    {
                        gCenterX = static_cast<float>(u);
                        gCenterY = static_cast<float>(overviewV);
                    }
                }
            }
            mouseWasDown = mouseDown;
            processInput(deltaTime);
            if (sample_input::pressed[GLFW_KEY_C])
                virtual_texture::clearDetail(virtualTexture);
            virtual_texture::update(virtualTexture, gCenterX, gCenterY, gViewSpan, gStreaming, gUploadBudget);
            glViewport(0, 0, width, height);

            // render
            // ------
            glClearColor(0.025f, 0.03f, 0.045f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();
            shader.setVec2("viewCenter", gCenterX, gCenterY);
            shader.setFloat("viewSpan", gViewSpan);
            shader.setInt("activeMip", virtualTexture.activeMip);
            virtual_texture::Page hoveredPage;
            hoveredPage.mip = -1;
            if (hoveredSlot >= 0 && virtualTexture.slots[hoveredSlot].page >= 0)
                hoveredPage = virtualTexture.pages[virtualTexture.slots[hoveredSlot].page];
            shader.setInt("hoveredSlot", hoveredSlot);
            shader.setInt("hoveredMip", hoveredPage.mip);
            if (hoveredPage.mip >= 0)
            {
                const float dimension = static_cast<float>(virtual_texture::VirtualPages >> hoveredPage.mip);
                shader.setVec4("hoveredBounds", hoveredPage.x / dimension, hoveredPage.y / dimension,
                               (hoveredPage.x + 1) / dimension, (hoveredPage.y + 1) / dimension);
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, virtualTexture.cache);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTable);
            glBindVertexArray(vao);
            // Draw the virtual image, its overview, then the physical backing cache.
            shader.setInt("displayMode", gResidency ? 1 : 0);
            glViewport(10, footer, std::max(1, left - 20), std::max(1, height - footer - 45));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            shader.setInt("displayMode", 3);
            glViewport(left + 5, overviewBottom, panelWidth, panelHeight);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            shader.setInt("displayMode", 2);
            glViewport(left + 5, footer, panelWidth, panelHeight);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            glViewport(0, 0, width, height);
            if (textReady)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                text.setWindowSize(width, height);
                const float scale = std::min(0.8f, width / 1500.0f);
                auto label = [&](const std::string &line, float x, float y)
                { text.RenderText(line, x, y, scale, {0.9f, 0.94f, 1.0f}); };
                label(gResidency ? "Virtual view: green = requested detail, orange = coarse fallback"
                                 : "NASA Blue Marble: streamed satellite image (V: residency colors)",
                      115, 17);
                label("Overview: click to jump; yellow = view", static_cast<float>(left + 10), 17);
                label("Physical cache: hover a slot to locate it", static_cast<float>(left + 10),
                      static_cast<float>(height - footer - panelHeight - 23));
                std::ostringstream memory, activity;
                memory << virtual_texture::VirtualSize << " x " << virtual_texture::VirtualSize << " RGBA8: " << std::fixed
                       << std::setprecision(0) << fullMiB << " MiB full base image vs " << std::setprecision(2) << cacheMiB
                       << " MiB cache + page table (allocated)";
                activity << (gStreaming ? "STREAMING" : "PAUSED") << " | resident " << virtualTexture.residentCount << "/"
                         << virtualTexture.slots.size() << " | pending " << virtualTexture.pendingCount << " | uploads "
                         << virtualTexture.uploadsThisFrame << "/" << gUploadBudget << " | evictions "
                         << virtualTexture.evictions << " | mip " << virtualTexture.activeMip << " | tile reads "
                         << virtualTexture.source.diskReads;
                label(memory.str(), 15, static_cast<float>(height - footer + 12));
                label(activity.str(), 15, static_cast<float>(height - footer + 38));
                std::ostringstream hoverInfo;
                if (hoveredSlot >= 0)
                {
                    hoverInfo << "Cache slot (" << hoveredSlot % virtual_texture::CachePages << ", "
                              << hoveredSlot / virtual_texture::CachePages << ")";
                    if (hoveredPage.mip < 0)
                        hoverInfo << " is empty; no image region is mapped.";
                    else
                    {
                        const int footprint = virtual_texture::PageSize << hoveredPage.mip;
                        hoverInfo << " -> mip " << hoveredPage.mip << " page (" << hoveredPage.x << ", " << hoveredPage.y
                                  << "), " << footprint << " x " << footprint << " image texels | cyan = page footprint";
                    }
                }
                else
                    hoverInfo << "Hover the physical cache to mark its page in cyan. Space pauses streaming; C clears "
                                 "detail; B slows uploads.";
                label(hoverInfo.str(), 15, static_cast<float>(height - footer + 64));
                label(
                    "WASD/arrows pan | wheel/Q/E zoom | click overview to jump | V colors | O overview | R Italy | H help",
                    15, static_cast<float>(height - footer + 90));
                label("NASA Blue Marble, July 2004 | 16K image tiles read from disk; full image stays out of GPU and CPU "
                      "memory.",
                      15, static_cast<float>(height - footer + 116));
                glDisable(GL_BLEND);
            }

            if (time >= nextTitleUpdate)
            {
                std::ostringstream title;
                title << "Virtual Texturing | cache " << virtualTexture.residentCount << "/" << virtualTexture.slots.size()
                      << ", uploads " << virtualTexture.uploadsThisFrame << "/frame"
                      << ", mip " << virtualTexture.activeMip << ", view " << std::fixed << std::setprecision(1)
                      << gViewSpan * 100.0f << "% | [WASD/arrows] pan [wheel/Q/E] zoom [R] reset [Esc] quit";
                glfwSetWindowTitle(window, title.str().c_str());
                nextTitleUpdate = time + 0.2;
            }
            // help overlay and swap buffers
            // -----------------------------
            sample_help::render();
            glfwSwapBuffers(window);
        }

        text.release();
        virtual_texture::destroy(virtualTexture);
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
        std::cerr << "Virtual texturing: " << error.what() << '\n';
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
    if (sample_input::pressed[GLFW_KEY_R])
    {
        gCenterX = StartX;
        gCenterY = StartY;
        gViewSpan = 0.025f;
    }

    if (sample_input::pressed[GLFW_KEY_SPACE])
        gStreaming = !gStreaming;
    if (sample_input::pressed[GLFW_KEY_V])
        gResidency = !gResidency;
    if (sample_input::pressed[GLFW_KEY_B])
        gUploadBudget = gUploadBudget == 4 ? 1 : 4;
    if (sample_input::pressed[GLFW_KEY_O])
    {
        gCenterX = gCenterY = 0.5f;
        gViewSpan = 1.0f;
    }
    const float movement = gViewSpan * deltaTime * 0.8f;
    if (sample_input::down[GLFW_KEY_A] || sample_input::down[GLFW_KEY_LEFT])
        gCenterX -= movement;
    if (sample_input::down[GLFW_KEY_D] || sample_input::down[GLFW_KEY_RIGHT])
        gCenterX += movement;
    if (sample_input::down[GLFW_KEY_S] || sample_input::down[GLFW_KEY_DOWN])
        gCenterY -= movement;
    if (sample_input::down[GLFW_KEY_W] || sample_input::down[GLFW_KEY_UP])
        gCenterY += movement;
    if (sample_input::down[GLFW_KEY_Q])
        gViewSpan *= std::pow(0.35f, deltaTime);
    if (sample_input::down[GLFW_KEY_E])
        gViewSpan *= std::pow(2.85f, deltaTime);
    if (gPendingScroll != 0.0)
    {
        gViewSpan *= std::pow(0.82f, static_cast<float>(gPendingScroll));
        gPendingScroll = 0.0;
    }
    gViewSpan = std::clamp(gViewSpan, 0.003f, 1.0f);
    const float halfSpan = gViewSpan * 0.5f;
    gCenterX = std::clamp(gCenterX, halfSpan, 1.0f - halfSpan);
    gCenterY = std::clamp(gCenterY, halfSpan, 1.0f - halfSpan);
}

void scroll_callback(GLFWwindow *, double, double yOffset)
{
    gPendingScroll += yOffset;
}
} // namespace
