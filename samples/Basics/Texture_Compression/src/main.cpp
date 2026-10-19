#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "compression_assets.h"
#include "compression_encoders.h"
#include "modules/filesystem.h"
#include "modules/shader_m.h"
#include "modules/window.h"
#include "modules/sample_input.h"
#include "modules/help_overlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
using compression::Format;
const std::array<Format, 7> Formats{
    {{"RGBA8", "landscape_rgba8.dds", "32 bits/texel. Full RGB and 8-bit alpha reference.", GL_RGBA8, 0, 4, 0, nullptr},
     {"DXT1 / BC1", "landscape_dxt1.dds",
      "4x4 / 64 bits: two RGB565 endpoints + sixteen 2-bit indices. RGB mode: alpha = 1.",
      GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 8, 1, nullptr},
     {"DXT3 / BC2", "landscape_dxt3.dds",
      "4x4 / 128 bits: 64-bit RGB + sixteen explicit 4-bit alpha values (16 opacity levels).",
      GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 4, 16, 3, nullptr},
     {"DXT5 / BC3", "landscape_dxt5.dds",
      "4x4 / 128 bits: 64-bit RGB + two alpha endpoints and sixteen interpolated 3-bit alpha indices.",
      GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 4, 16, 5, nullptr},
     {"ASTC 4x4", "landscape_4x4.astc",
      "128 bits per 4x4 block: 8 bits/texel; RGBA. Offline Arm astcenc, medium preset.",
      GL_COMPRESSED_RGBA_ASTC_4x4_KHR, 4, 16, 0, "preview_4x4.png"},
     {"ASTC 8x8", "landscape_8x8.astc",
      "128 bits per 8x8 block: 2 bits/texel; fewer bytes, but fine structures can lose detail.",
      GL_COMPRESSED_RGBA_ASTC_8x8_KHR, 8, 16, 0, "preview_8x8.png"},
     {"ASTC 12x12", "landscape_12x12.astc",
      "128 bits per 12x12 block: about 0.89 bits/texel; partial edge blocks are padded.",
      GL_COMPRESSED_RGBA_ASTC_12x12_KHR, 12, 16, 0, "preview_12x12.png"}}};
struct Variant
{
    GLuint texture = 0;
    bool supported = false;
    std::string status;
    compression::Measurement timing;
};
int gSelected = 3;
bool gAlpha = false, gDifference = false, gBlocks = false;
float gZoom = 1.0f, gCenterX = 0.5f, gCenterY = 0.5f;

std::string number(double value, int precision = 3)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(precision) << value;
    return text.str();
}
std::string sizeLabel(std::size_t bytes)
{
    return number(bytes / 1024.0, bytes % 1024 == 0 ? 0 : 1) + " KiB";
}
void scrollCallback(GLFWwindow *, double, double offset)
{
    gZoom = std::clamp(gZoom * std::pow(1.3f, static_cast<float>(offset)), 1.0f, 32.0f);
}
void processInput(float dt)
{
    for (int i = 0; i < static_cast<int>(Formats.size()); ++i)
        if (sample_input::pressed[GLFW_KEY_1 + i])
            gSelected = i;
    if (sample_input::pressed[GLFW_KEY_A])
        gAlpha = !gAlpha;
    if (sample_input::pressed[GLFW_KEY_D])
        gDifference = !gDifference;
    if (sample_input::pressed[GLFW_KEY_B])
        gBlocks = !gBlocks;
    if (sample_input::pressed[GLFW_KEY_Z])
        gZoom = gZoom < 4.0f ? 4.0f : (gZoom < 16.0f ? 16.0f : 1.0f);
    if (sample_input::pressed[GLFW_KEY_R])
    {
        gZoom = 1.0f;
        gCenterX = gCenterY = 0.5f;
        gAlpha = gDifference = gBlocks = false;
    }
    const float step = dt * 0.6f / gZoom;
    if (sample_input::down[GLFW_KEY_LEFT])
        gCenterX -= step;
    if (sample_input::down[GLFW_KEY_RIGHT])
        gCenterX += step;
    if (sample_input::down[GLFW_KEY_UP])
        gCenterY += step;
    if (sample_input::down[GLFW_KEY_DOWN])
        gCenterY -= step;
    const float halfSpan = 0.5f / gZoom;
    gCenterX = std::clamp(gCenterX, halfSpan, 1.0f - halfSpan);
    gCenterY = std::clamp(gCenterY, halfSpan, 1.0f - halfSpan);
}
GLuint createPreview(const Format &format, const compression::Asset &asset, const std::string &directory)
{
    compression::Asset decoded;
    if (format.dxt)
        decoded.payload =
            compression::decodeDxt(asset.payload, compression::ImageWidth, compression::ImageHeight, format.dxt);
    else
    {
        int width, height, channels;
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
            stbi_load((directory + "/" + format.preview).c_str(), &width, &height, &channels, 4), stbi_image_free);
        if (!pixels || width != compression::ImageWidth || height != compression::ImageHeight)
            throw std::runtime_error("Cannot load offline ASTC preview");
        decoded.payload.assign(pixels.get(), pixels.get() + width * height * 4);
    }
    const GLuint texture = compression::uploadAsset(Formats[0], decoded);
    if (!texture)
        throw std::runtime_error("Cannot upload RGBA8 preview");
    return texture;
}
void runBenchmark(const std::string &directory, std::array<Variant, 7> &variants)
{
    std::cout << "\nPrepared assets: median of 7 warm-cache read + completed upload trials (ms).\n";
    for (std::size_t i = 0; i < Formats.size(); ++i)
    {
        variants[i].timing = compression::benchmark(directory, Formats[i], variants[i].supported);
        const auto &t = variants[i].timing;
        std::cout << Formats[i].name << ": payload " << compression::payloadSize(Formats[i]) << " B | read "
                  << t.readMs;
        if (variants[i].supported)
            std::cout << " | upload " << t.uploadMs << " | load " << t.totalMs;
        else
            std::cout << " | upload/load N/A (RGBA8 preview)";
        std::cout << '\n';
    }
}
} // namespace

int runSample()
{
    GLFWwindow *window = sample_window::create(1400, 980, "Texture Compression: offline assets, measured loading");
    if (!window)
        return -1;
    glfwSetScrollCallback(window, scrollCallback);
    sample_help::initialize(window, {{"1 / 2 / 3 / 4", "RGBA8 / DXT1 / DXT3 / DXT5"},
                                     {"5 / 6 / 7", "ASTC 4x4 / 8x8 / 12x12"},
                                     {"A", "Show alpha channel"},
                                     {"D", "Show amplified error"},
                                     {"B", "Show compression block grid"},
                                     {"Wheel / Z", "Zoom / cycle 1x, 4x, 16x"},
                                     {"Arrow keys", "Pan zoomed image"},
                                     {"T", "Repeat loading measurements"},
                                     {"R", "Reset image view"},
                                     {"Esc", "Exit sample"}});
    TextRenderer text;
    const bool textReady = text.Load(FileSystem::getPath("content/fonts/GO-Regular.ttf"), 20);
    const std::string directory = FileSystem::getPath("content/images/texture_compression");
    std::array<Variant, 7> variants{};
    for (std::size_t i = 0; i < Formats.size(); ++i)
    {
        const auto asset = compression::readAsset(directory, Formats[i]);
        const bool extension = i == 0 || (Formats[i].dxt ? GLAD_GL_EXT_texture_compression_s3tc != 0
                                                         : GLAD_GL_KHR_texture_compression_astc_ldr != 0);
        if (extension)
            variants[i].texture = compression::uploadAsset(Formats[i], asset);
        variants[i].supported = variants[i].texture != 0;
        if (!variants[i].supported)
        {
            if (i == 0)
                throw std::runtime_error("RGBA8 source upload failed");
            variants[i].status = extension ? "driver rejected compressed upload" : "GPU format unavailable";
            variants[i].texture = createPreview(Formats[i], asset, directory);
        }
        else
            variants[i].status = "native GPU-compressed sampling";
    }
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << '\n';
    runBenchmark(directory, variants);

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
    Shader shader(FileSystem::getSamplePath("shader/compression.vert").c_str(),
                  FileSystem::getSamplePath("shader/compression.frag").c_str());
    GLint linked = GL_FALSE;
    glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
    if (!linked)
        throw std::runtime_error("Texture sample shader could not be linked; check the sidecar files");
    shader.use();
    shader.setInt("displayTexture", 0);
    shader.setInt("referenceTexture", 1);
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        sample_input::poll(window);
        const double now = glfwGetTime();
        processInput(static_cast<float>(std::min(0.1, now - lastTime)));
        lastTime = now;
        if (sample_input::pressed[GLFW_KEY_T])
        {
            glfwSetWindowTitle(window, "Texture Compression: measuring prepared texture loads...");
            runBenchmark(directory, variants);
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0)
            continue;
        const float uiScale = std::min(width / 1400.0f, height / 800.0f);
        const int footer = static_cast<int>(345 * uiScale);
        const int imageSize = std::max(1, std::min(width / 2 - 24, std::max(1, height - footer - 55)));
        glViewport(0, 0, width, height);
        glClearColor(0.025f, 0.03f, 0.045f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        shader.setBool("showAlpha", gAlpha);
        shader.setBool("showDifference", gDifference);
        shader.setBool("showBlocks", gBlocks);
        shader.setFloat("blockSize", static_cast<float>(std::max(4, Formats[gSelected].blockSize)));
        shader.setFloat("zoom", gZoom);
        shader.setVec2("viewCenter", gCenterX, gCenterY);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, variants[0].texture);
        glBindVertexArray(vao);
        for (int panel = 0; panel < 2; ++panel)
        {
            shader.setBool("comparison", panel == 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, variants[panel == 0 ? 0 : gSelected].texture);
            glViewport(panel * (width / 2) + (width / 2 - imageSize) / 2,
                       footer + (height - footer - 45 - imageSize) / 2, imageSize, imageSize);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }
        glViewport(0, 0, width, height);
        if (textReady)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            text.setWindowSize(width, height);
            auto label = [&](const std::string &line, float x, float y, glm::vec3 color = glm::vec3(0.88f, 0.93f, 1.0f))
            { text.RenderText(line, x * uiScale, y * uiScale, 0.76f * uiScale, color); };
            const float top = (height - footer) / uiScale;
            label(gAlpha ? "REFERENCE: RGBA8 alpha" : "REFERENCE: RGBA8 photograph + alpha band", 115, 19);
            std::string selected = Formats[gSelected].name;
            if (!variants[gSelected].supported)
                selected += " (RGBA8 decoded preview)";
            if (gDifference)
                selected += gAlpha ? " | alpha error x16" : " | RGB error x8";
            label(selected, width / (2 * uiScale) + 20, 19);
            label("GPU-ready texture payloads | 1024 x 1024 | base level only | median of 7 warm-cache load trials", 15,
                  top + 10);
            const float header = top + 36;
            label("Format", 25, header);
            label("Block", 210, header);
            label("bpt", 300, header);
            label("Payload", 365, header);
            label("Ratio", 500, header);
            label("Saved", 590, header);
            label("Read ms", 710, header);
            label("Upload ms", 850, header);
            label("Load ms", 1005, header);
            label("GPU payload", 1165, header);
            for (int i = 0; i < static_cast<int>(Formats.size()); ++i)
            {
                const auto &format = Formats[i];
                const auto &variant = variants[i];
                const auto bytes = compression::payloadSize(format);
                const double ratio = static_cast<double>(compression::payloadSize(Formats[0])) / bytes;
                const float y = top + 62 + i * 23;
                if (i == gSelected)
                    text.RenderBox(10 * uiScale, (y - 3) * uiScale, width - 20 * uiScale, 23 * uiScale,
                                   {0.07f, 0.18f, 0.28f, 1.0f});
                label("[" + std::to_string(i + 1) + "] " + format.name, 25, y);
                label(format.blockSize ? std::to_string(format.blockSize) + "x" + std::to_string(format.blockSize)
                                       : "--",
                      210, y);
                const double bpt =
                    format.blockSize ? 8.0 * format.bytesPerBlock / (format.blockSize * format.blockSize) : 32.0;
                label(number(bpt, bpt == std::floor(bpt) ? 0 : 2), 300, y);
                label(sizeLabel(bytes), 365, y);
                label(number(ratio, ratio == std::floor(ratio) ? 0 : 1) + ":1", 500, y);
                label(number(100.0 * (1.0 - 1.0 / ratio), 1) + "%", 590, y);
                label(number(variant.timing.readMs), 710, y);
                label(variant.supported ? number(variant.timing.uploadMs) : "N/A", 850, y);
                label(variant.supported ? number(variant.timing.totalMs) : "N/A", 1005, y);
                label(sizeLabel(variant.supported ? bytes : compression::payloadSize(Formats[0])), 1165, y);
            }
            label(Formats[gSelected].description, 15, top + 229, {0.5f, 0.8f, 1.0f});
            const auto selectedBytes = compression::payloadSize(Formats[gSelected]);
            std::string delta = "RGBA8 " + sizeLabel(compression::payloadSize(Formats[0])) + " -> " +
                                sizeLabel(selectedBytes) + " (saved " +
                                sizeLabel(compression::payloadSize(Formats[0]) - selectedBytes) + "): ";
            if (variants[gSelected].supported)
            {
                const double percent = 100.0 * (1.0 - variants[gSelected].timing.totalMs / variants[0].timing.totalMs);
                delta += "measured load " + number(std::abs(percent), 1) + (percent >= 0 ? "% faster" : "% slower") +
                         " than RGBA8 on this run.";
            }
            else
                delta += "compressed GPU load unavailable; displayed preview occupies 4096 KiB RGBA8.";
            label(delta, 15, top + 253);
            label("Read = file open + read + header parse/allocation. Upload = GL allocation/transfer + glFinish. Load "
                  "= read + upload.",
                  15, top + 277);
            label(variants[gSelected].supported
                      ? "Measured warm-cache wall times; encoding is offline. Results vary with driver/cache state. T "
                        "repeats timings."
                      : variants[gSelected].status +
                            "; quality preview decoded to RGBA8. Compressed GPU upload/load timings are N/A.",
                  15, top + 301);
            label("Photo: Hannes Roest / Wikimedia Commons, CC BY-SA 3.0 | Bottom quarter: added alpha ramp | 1-7 "
                  "format, A alpha, D error, Z zoom, B blocks, H help",
                  15, top + 325);
            glDisable(GL_BLEND);
        }
        const std::string title = std::string("Texture Compression | ") + Formats[gSelected].name +
                                  " | [1-7] format [A] alpha [D] error [T] timings [H] help";
        glfwSetWindowTitle(window, title.c_str());
        sample_help::render();
        glfwSwapBuffers(window);
    }
    text.release();
    sample_help::shutdown();
    for (auto &variant : variants)
        glDeleteTextures(1, &variant.texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
int main()
{
    try
    {
        return runSample();
    }
    catch (const std::exception &error)
    {
        std::cerr << "Texture compression: " << error.what() << '\n';
        sample_help::shutdown();
        glfwTerminate();
        return -1;
    }
}
