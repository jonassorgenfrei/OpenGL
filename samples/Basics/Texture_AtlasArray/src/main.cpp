#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/window.h"
#include "modules/sample_input.h"
#include "modules/help_overlay.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
// settings
constexpr unsigned int SCR_WIDTH = 1200;
constexpr unsigned int SCR_HEIGHT = 800;

constexpr int TileSize = 256;
constexpr int TileCount = 4;
constexpr int MaxAtlasLod = 9;
const std::array<const char *, TileCount> MaterialNames = {"Stone", "Grass", "Wood", "Metal"};
const std::array<const char *, TileCount> MaterialPaths = {
    "content/images/brickwall.jpg", "content/images/grass/albedo.png",
    "content/images/wood.png", "content/images/metal.png"};

// Input, callbacks and rendering helpers are defined after main().
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
std::vector<std::uint8_t> loadTile(int tile);
void configureTexture(GLenum target);
void processInput(int &lod, bool &inset, bool &storageView, bool &repeat);
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
        GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Texture Atlas / Texture Array", nullptr, nullptr);
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
        sample_help::initialize(window, {{"Esc", "Exit sample"},
                                         {"1 / 2", "Material detail / mip bleeding comparison"},
                                         {"Up / Down", "Increase / decrease mip level"},
                                         {"G", "Atlas base-level half-texel inset"},
                                         {"T", "Repeat tiles 1 / 4 times"},
                                         {"M", "Material grid / storage view"},
                                         {"R", "Reset comparison"}});
        TextRenderer text;
        const bool textReady = text.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20);

        GLuint atlas = 0, tileArray = 0;
        std::vector<std::uint8_t> atlasPixels(TileSize * 2 * TileSize * 2 * 4);
        std::array<std::vector<std::uint8_t>, TileCount> tiles;
        for (int tile = 0; tile < TileCount; ++tile)
        {
            tiles[tile] = loadTile(tile);
            for (int y = 0; y < TileSize; ++y)
                for (int x = 0; x < TileSize; ++x)
                {
                    const int destination = ((tile / 2 * TileSize + y) * TileSize * 2 + tile % 2 * TileSize + x) * 4;
                    std::copy_n(&tiles[tile][(y * TileSize + x) * 4], 4, &atlasPixels[destination]);
                }
        }
        // Atlas: one 512x512 image, each material selected by UV scale and offset.
        glGenTextures(1, &atlas);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TileSize * 2, TileSize * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     atlasPixels.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Array: four equally sized 256x256 layers, selected by a third coordinate.
        glGenTextures(1, &tileArray);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, tileArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TileSize, TileSize, TileCount, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);
        for (int tile = 0; tile < TileCount; ++tile)
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, tile, TileSize, TileSize, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            tiles[tile].data());

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        // set up vertex data and configure vertex attributes
        // --------------------------------------------------
        const float vertices[] = {-1, -1, 0, 0, 1, -1, 1, 0, 1, 1, 1, 1, -1, 1, 0, 1};
        const unsigned int indices[] = {0, 1, 2, 0, 2, 3};
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // build and compile our shader program
        // ------------------------------------
        Shader shader(FileSystem::getSamplePath("shader/atlas_array.vert").c_str(),
                      FileSystem::getSamplePath("shader/atlas_array.frag").c_str());
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
        if (!linked)
            throw std::runtime_error("Texture sample shader could not be linked; check the sidecar files");
        shader.use();
        shader.setInt("atlas", 0);
        shader.setInt("tileArray", 1);
        int lod = 0;
        bool inset = false, storageView = false, repeat = false;
        std::cout << "Atlas / array: identical stone, grass, wood and metal; 1 MiB base payload each.\n";
        // render loop
        // -----------
        while (!glfwWindowShouldClose(window))
        {
            // input
            // -----
            sample_input::poll(window);
            processInput(lod, inset, storageView, repeat);
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwWaitEventsTimeout(0.05);
                continue;
            }
            const int footer = std::min(154, height / 3);
            const int columnWidth = width / 3;
            const int panelWidth = std::max(1, columnWidth - 24);
            const int panelHeight = std::max(1, height - footer - 55);
            const int previewSize = std::min(panelWidth, panelHeight);
            glViewport(0, 0, width, height);
            // render
            // ------
            glClearColor(0.025f, 0.03f, 0.045f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();
            shader.setFloat("lod", static_cast<float>(lod));
            shader.setFloat("repetitions", repeat ? 4.0f : 1.0f);
            shader.setBool("inset", inset);
            shader.setBool("storageView", storageView);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, atlas);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D_ARRAY, tileArray);
            glBindVertexArray(vao);
            // Show the complete base-level atlas independently of comparison settings.
            // UVs cover [0,1] without tile selection, repetition or inset.
            shader.setBool("useArray", false);
            shader.setBool("storageView", true);
            shader.setFloat("lod", 0.0f);
            glViewport(12 + (panelWidth - previewSize) / 2,
                       footer + (panelHeight - previewSize) / 2, previewSize, previewSize);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            shader.setBool("storageView", storageView);
            shader.setFloat("lod", static_cast<float>(lod));
            for (int panel = 0; panel < 2; ++panel)
            {
                shader.setBool("useArray", panel == 1);
                glViewport((panel + 1) * columnWidth + 12, footer, panelWidth, panelHeight);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
            glViewport(0, 0, width, height);
            if (textReady)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                text.setWindowSize(width, height);
                const float scale = std::min(0.8f, width / 1500.0f);
                auto label = [&](const std::string &line, float x, float y)
                { text.RenderText(line, x, y, scale, {0.9f, 0.94f, 1.0f}); };
                label("FULL ATLAS: 512 x 512", 15, 20);
                label("ATLAS: UV rectangle", columnWidth + 15.0f, 20);
                label("ARRAY: UV + layer", 2 * columnWidth + 15.0f, 20);
                label("All four materials | mip 0", 15, 40);
                for (int panel = 0; panel < 2; ++panel)
                    for (int tile = 0; tile < TileCount; ++tile)
                    {
                        const float x = (panel + 1) * columnWidth + 20.0f + (tile % 2) * panelWidth / 2.0f;
                        const float y = 65 + (1 - tile / 2) * panelHeight / 2.0f;
                        const std::string name = std::string(MaterialNames[tile]) +
                            (panel == 0 ? " | tile " : " | layer ") + std::to_string(tile);
                        text.RenderText(name, x + 1, y + 1, scale * 0.85f, {0.0f, 0.0f, 0.0f});
                        text.RenderText(name, x, y, scale * 0.85f, {0.9f, 0.94f, 1.0f});
                    }
                label("Same materials: 512x512 atlas vs four 256x256 layers; 1 MiB base payload each.", 15,
                      static_cast<float>(height - footer + 12));
                label("Mip level: " + std::to_string(lod) + " (array max 8) | Atlas base-level inset: " +
                          (inset ? "ON" : "OFF") + " | Tile repeats: " + (repeat ? "4" : "1") + " | " +
                          (storageView ? "STORAGE VIEW" : "MATERIAL GRID"),
                      15, static_cast<float>(height - footer + 38));
                label("Look for grass leaking into stone or wood at atlas edges; array layers stay separate.", 15,
                      static_cast<float>(height - footer + 64));
                label("Inset fixes base filtering. Coarser atlas mips need padding and per-tile mip generation.",
                      15, static_cast<float>(height - footer + 90));
                label("Atlas packs different rectangles; arrays require equal layer size/format and isolate each tile.", 15,
                      static_cast<float>(height - footer + 116));
                glDisable(GL_BLEND);
            }
            const std::string title =
                "Texture Atlas / Array | mip " + std::to_string(lod);
            glfwSetWindowTitle(window, title.c_str());
            // help overlay and swap buffers
            // -----------------------------
            sample_help::render(55); // Keep the hint and expanded help below the column headers.
            glfwSwapBuffers(window);
        }
        text.release();
        sample_help::shutdown();
        glDeleteTextures(1, &atlas);
        glDeleteTextures(1, &tileArray);
        // release geometry resources
        // --------------------------
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteProgram(shader.ID);
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

std::vector<std::uint8_t> loadTile(int tile)
{
    // Normalize the existing material images once; both storage methods receive
    // exactly the same pixels. Box filtering preserves detail when downsampling.
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    const std::string path = FileSystem::getPath(MaterialPaths[tile]);
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> source(
        stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
    if (!source)
        throw std::runtime_error("Could not load material texture: " + path);
    std::vector<std::uint8_t> pixels(TileSize * TileSize * 4);
    for (int y = 0; y < TileSize; ++y)
        for (int x = 0; x < TileSize; ++x)
        {
            const int x0 = x * width / TileSize, y0 = y * height / TileSize;
            const int x1 = std::max(x0 + 1, (x + 1) * width / TileSize);
            const int y1 = std::max(y0 + 1, (y + 1) * height / TileSize);
            std::array<std::uint64_t, 4> sum{};
            for (int sy = y0; sy < y1; ++sy)
                for (int sx = x0; sx < x1; ++sx)
                    for (int c = 0; c < 4; ++c)
                        sum[c] += source.get()[(sy * width + sx) * 4 + c];
            const int count = (x1 - x0) * (y1 - y0);
            for (int c = 0; c < 4; ++c)
                pixels[(y * TileSize + x) * 4 + c] = static_cast<std::uint8_t>(sum[c] / count);
        }
    return pixels;
}


void processInput(int &lod, bool &inset, bool &storageView, bool &repeat)
{
    if (sample_input::pressed[GLFW_KEY_UP])
        lod = std::min(MaxAtlasLod, lod + 1);
    if (sample_input::pressed[GLFW_KEY_DOWN])
        lod = std::max(0, lod - 1);
    if (sample_input::pressed[GLFW_KEY_G])
        inset = !inset;
    if (sample_input::pressed[GLFW_KEY_T])
        repeat = !repeat;
    if (sample_input::pressed[GLFW_KEY_M])
        storageView = !storageView;
    if (sample_input::pressed[GLFW_KEY_R] || sample_input::pressed[GLFW_KEY_1])
    {
        lod = 0;
        inset = false;
        storageView = false;
        repeat = false;
    }
    if (sample_input::pressed[GLFW_KEY_2])
    {
        lod = 6;
        inset = false;
        storageView = false;
        repeat = true;
    }
}
} // namespace
