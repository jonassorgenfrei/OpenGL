#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"
#include "modules/help_overlay.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr int TileSize = 64;
constexpr int TileCount = 4;
std::array<bool, GLFW_KEY_LAST + 1> previousKeys{};
bool pressedOnce(GLFWwindow* window, int key)
{
    const bool down = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressed = down && !previousKeys[key];
    previousKeys[key] = down;
    return pressed;
}

std::vector<std::uint8_t> makeTile(int tile)
{
    const std::array<std::array<std::uint8_t, 3>, 4> colors = {{
        {{235, 65, 45}}, {{45, 210, 95}}, {{50, 100, 240}}, {{245, 200, 40}}
    }};
    std::vector<std::uint8_t> pixels(TileSize * TileSize * 4);
    for (int y = 0; y < TileSize; ++y)
        for (int x = 0; x < TileSize; ++x)
        {
            const bool checker = ((x / 8 + y / 8) & 1) != 0;
            // Solid tile-colored edges make contamination from adjacent tiles visible.
            const float brightness = x < 4 || y < 4 || x >= 60 || y >= 60 ? 1.0f : (checker ? 0.85f : 0.45f);
            const int offset = (y * TileSize + x) * 4;
            for (int c = 0; c < 3; ++c)
                pixels[offset + c] = static_cast<std::uint8_t>(colors[tile][c] * brightness);
            pixels[offset + 3] = 255;
        }
    return pixels;
}

void configureTexture(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(target);
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
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Texture Atlas / Texture Array", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    icon(window);
    sample_help::initialize(window, {
        {"Esc", "Exit sample"}, {"Up / Down", "Increase / decrease mip level"},
        {"G", "Atlas base-level half-texel inset"}, {"T", "Repeat tiles 1 / 4 times"},
        {"M", "Material grid / storage view"}, {"R", "Reset comparison"}
    });
    TextRenderer text;
    const bool textReady = text.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20);

    GLuint atlas = 0, tileArray = 0;
    std::vector<std::uint8_t> atlasPixels(TileSize * 2 * TileSize * 2 * 4);
    std::array<std::vector<std::uint8_t>, TileCount> tiles;
    for (int tile = 0; tile < TileCount; ++tile)
    {
        tiles[tile] = makeTile(tile);
        for (int y = 0; y < TileSize; ++y)
            for (int x = 0; x < TileSize; ++x)
            {
                const int destination = ((tile / 2 * TileSize + y) * TileSize * 2 + tile % 2 * TileSize + x) * 4;
                std::copy_n(&tiles[tile][(y * TileSize + x) * 4], 4, &atlasPixels[destination]);
            }
    }
    // Atlas: one 128x128 image, each tile selected by UV scale and offset.
    glGenTextures(1, &atlas);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TileSize * 2, TileSize * 2, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, atlasPixels.data());
    configureTexture(GL_TEXTURE_2D);

    // Array: four equally sized 64x64 layers, selected by a third coordinate.
    glGenTextures(1, &tileArray);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tileArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TileSize, TileSize, TileCount, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    for (int tile = 0; tile < TileCount; ++tile)
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, tile, TileSize, TileSize, 1,
                       GL_RGBA, GL_UNSIGNED_BYTE, tiles[tile].data());
    configureTexture(GL_TEXTURE_2D_ARRAY);

    const float vertices[] = {-1,-1,0,0, 1,-1,1,0, 1,1,1,1, -1,1,0,1};
    const unsigned int indices[] = {0,1,2, 0,2,3};
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    Shader shader(FileSystem::getSamplePath("shader/atlas_array.vert").c_str(),
                  FileSystem::getSamplePath("shader/atlas_array.frag").c_str());
    shader.use(); shader.setInt("atlas", 0); shader.setInt("tileArray", 1);
    int lod = 3;
    bool inset = false, storageView = false, repeat = true;
    std::cout << "Atlas / array: identical four tiles, 64 KiB base payload each.\n"
              << "Up/Down: mip  G: inset  T: repetition  M: storage  R: reset  H: help\n";
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        if (pressedOnce(window, GLFW_KEY_UP)) lod = std::min(7, lod + 1);
        if (pressedOnce(window, GLFW_KEY_DOWN)) lod = std::max(0, lod - 1);
        if (pressedOnce(window, GLFW_KEY_G)) inset = !inset;
        if (pressedOnce(window, GLFW_KEY_T)) repeat = !repeat;
        if (pressedOnce(window, GLFW_KEY_M)) storageView = !storageView;
        if (pressedOnce(window, GLFW_KEY_R)) { lod = 3; inset = false; storageView = false; repeat = true; }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) continue;
        const int footer = std::min(180, height / 3);
        glViewport(0, 0, width, height);
        glClearColor(0.025f, 0.03f, 0.045f, 1); glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        shader.setFloat("lod", static_cast<float>(lod));
        shader.setFloat("repetitions", repeat ? 4.0f : 1.0f);
        shader.setBool("inset", inset); shader.setBool("storageView", storageView);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlas);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, tileArray);
        glBindVertexArray(vao);
        for (int panel = 0; panel < 2; ++panel)
        {
            shader.setBool("useArray", panel == 1);
            glViewport(panel * (width / 2) + 12, footer, std::max(1, width / 2 - 24), std::max(1, height - footer - 55));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
        glViewport(0, 0, width, height);
        if (textReady)
        {
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            text.setWindowSize(width, height);
            const float scale = std::min(0.8f, width / 1500.0f);
            auto label = [&](const std::string& line, float x, float y) {
                text.RenderText(line, x, y, scale, {0.9f, 0.94f, 1.0f});
            };
            label("ATLAS: sampler2D, UV rectangle", 115, 20);
            label("ARRAY: sampler2DArray, UV + layer", width / 2.0f + 20, 20);
            label("Same four tiles; one texture binding per approach; 64 KiB base image payload each.", 15, static_cast<float>(height - footer + 12));
            label("Mip level: " + std::to_string(lod) + " (array max 6) | Atlas base-level inset: " + (inset ? "ON" : "OFF") +
                  " | Tile repeats: " + (repeat ? "4" : "1") + " | " + (storageView ? "STORAGE VIEW" : "MATERIAL GRID"), 15, static_cast<float>(height - footer + 38));
            label("Observe colored seams in the atlas. Array filtering and mipmaps stay inside each layer.", 15, static_cast<float>(height - footer + 64));
            label("Try mip 0 + G: inset fixes base filtering. Raise mip: atlas needs padding and per-tile mip generation.", 15, static_cast<float>(height - footer + 90));
            label("Atlas packs different rectangles; arrays require equal layer size/format and isolate each tile.", 15, static_cast<float>(height - footer + 116));
            label("Up/Down mip | G inset | T repeat | M storage layout | R reset | H help | Esc exit", 15, static_cast<float>(height - footer + 142));
            glDisable(GL_BLEND);
        }
        const std::string title = "Texture Atlas / Array | mip " + std::to_string(lod) + " | G inset | T repeat | M storage | H help";
        glfwSetWindowTitle(window, title.c_str());
        sample_help::render();
        glfwSwapBuffers(window);
    }
    text.release(); sample_help::shutdown();
    glDeleteTextures(1, &atlas); glDeleteTextures(1, &tileArray);
    glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
