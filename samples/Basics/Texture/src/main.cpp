#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "modules/filesystem.h"
#include "modules/shader_s.h"
#include "modules/window.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
// The source image is deliberately small enough to compare formats directly.
// The virtual image is larger, but only CACHE_PAGES squared pages ever reside
// in GPU memory. Each physical page includes a one-texel filtering gutter.
constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 700;
constexpr int IMAGE_SIZE = 256;
constexpr int ASTC_BLOCK = 4;
constexpr int VIRTUAL_SIZE = 2048;
constexpr int PAGE_SIZE = 64;
constexpr int VIRTUAL_PAGES = VIRTUAL_SIZE / PAGE_SIZE;
constexpr int VIRTUAL_MIP_LEVELS = 6; // 32, 16, 8, 4, 2, and 1 pages per axis
constexpr int PINNED_MIP = 4;         // 2x2 and 1x1 fallbacks never leave memory
constexpr int PAGE_TABLE_HEIGHT = 63; // sum of all mip dimensions
constexpr int CACHE_PAGES = 11;
constexpr int PAGE_GUTTER = 1;
constexpr int SLOT_SIZE = PAGE_SIZE + PAGE_GUTTER * 2;
constexpr int CACHE_SIZE = CACHE_PAGES * SLOT_SIZE;
constexpr int UPLOADS_PER_FRAME = 4;

enum class Mode { Source, Dxt1, Astc, Virtual };

struct VirtualTexture
{
    // Logical pages point into the physical cache. A value of -1 means that
    // the shader falls back to a coarser resident mip until this page streams.
    struct Page
    {
        int slot = -1;
        int mip = 0;
        int x = 0;
        int y = 0;
    };

    // Cache slots track ownership and age for least-recently-used replacement.
    struct Slot { int page = -1; std::uint64_t lastUsed = 0; };

    GLuint cache = 0;
    GLuint pageTable = 0;
    std::vector<Page> pages;
    std::array<Slot, CACHE_PAGES * CACHE_PAGES> slots{};
    std::vector<std::uint8_t> tablePixels;
    std::uint64_t frame = 0;
    int residentCount = 0;
    int uploadsThisFrame = 0;
    int activeMip = 0;
};

struct PageRect
{
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;

    int count() const { return (maxX - minX + 1) * (maxY - minY + 1); }
};

Mode gMode = Mode::Virtual;
float gCenterX = 0.5f;
float gCenterY = 0.5f;
float gViewSpan = 0.20f;
double gPendingScroll = 0.0;
std::array<bool, GLFW_KEY_LAST + 1> gPreviousKeys{};

float clampFloat(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

int mipPageDimension(int mip)
{
    return std::max(1, VIRTUAL_PAGES >> mip);
}

int pageTableYOffset(int mip)
{
    int offset = 0;
    for (int level = 0; level < mip; ++level) offset += mipPageDimension(level);
    return offset;
}

int mipPageOffset(int mip)
{
    int offset = 0;
    for (int level = 0; level < mip; ++level)
    {
        const int dimension = mipPageDimension(level);
        offset += dimension * dimension;
    }
    return offset;
}

int pageIndex(int mip, int x, int y)
{
    return mipPageOffset(mip) + y * mipPageDimension(mip) + x;
}

std::size_t pageTablePixelOffset(const VirtualTexture::Page& page)
{
    const int tableY = pageTableYOffset(page.mip) + page.y;
    return static_cast<std::size_t>((tableY * VIRTUAL_PAGES + page.x) * 4);
}

PageRect requestedPageRect(int mip)
{
    const int dimension = mipPageDimension(mip);
    const float halfSpan = gViewSpan * 0.5f;
    return {
        std::max(0, static_cast<int>(std::floor((gCenterX - halfSpan) * dimension)) - 1),
        std::min(dimension - 1, static_cast<int>(std::floor((gCenterX + halfSpan) * dimension)) + 1),
        std::max(0, static_cast<int>(std::floor((gCenterY - halfSpan) * dimension)) - 1),
        std::min(dimension - 1, static_cast<int>(std::floor((gCenterY + halfSpan) * dimension)) + 1)
    };
}

std::uint8_t toByte(float value)
{
    return static_cast<std::uint8_t>(clampFloat(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

// Gradients, a grid, a checker, and rings expose compression artifacts and
// virtual page boundaries more clearly than a smooth source image would.
std::array<std::uint8_t, 4> proceduralColor(int x, int y, int size)
{
    x = std::max(0, std::min(size - 1, x));
    y = std::max(0, std::min(size - 1, y));
    const float u = static_cast<float>(x) / static_cast<float>(size - 1);
    const float v = static_cast<float>(y) / static_cast<float>(size - 1);
    const int cellX = x / std::max(1, size / 16);
    const int cellY = y / std::max(1, size / 16);
    const float checker = ((cellX + cellY) & 1) ? 0.14f : 0.0f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float rings = 0.5f + 0.5f * std::sin(std::sqrt(dx * dx + dy * dy) * 95.0f);
    const int gridStep = std::max(1, size / 8);
    const int gridWidth = std::max(1, size / 256);
    const bool majorGrid = x % gridStep < gridWidth || y % gridStep < gridWidth;

    float r = 0.10f + 0.70f * u + checker;
    float g = 0.10f + 0.70f * v + checker;
    float b = 0.16f + 0.34f * rings + checker;
    if (majorGrid) r = g = b = 0.96f;
    return { toByte(r), toByte(g), toByte(b), 255 };
}

std::vector<std::uint8_t> makeSourceImage()
{
    std::vector<std::uint8_t> pixels(IMAGE_SIZE * IMAGE_SIZE * 4);
    for (int y = 0; y < IMAGE_SIZE; ++y)
    {
        for (int x = 0; x < IMAGE_SIZE; ++x)
        {
            const auto color = proceduralColor(x, y, IMAGE_SIZE);
            std::copy(color.begin(), color.end(), pixels.begin() + (y * IMAGE_SIZE + x) * 4);
        }
    }
    return pixels;
}

// DXT1 stores two RGB565 endpoints and sixteen two-bit palette indices per
// 4x4 block. This teaching encoder favors clarity over exhaustive endpoint search.
std::uint16_t packRgb565(const std::uint8_t* color)
{
    return static_cast<std::uint16_t>(((color[0] >> 3) << 11) |
                                      ((color[1] >> 2) << 5) |
                                      (color[2] >> 3));
}

std::array<int, 3> unpackRgb565(std::uint16_t color)
{
    return {
        static_cast<int>(((color >> 11) & 31) * 255 / 31),
        static_cast<int>(((color >> 5) & 63) * 255 / 63),
        static_cast<int>((color & 31) * 255 / 31)
    };
}

std::vector<std::uint8_t> encodeDxt1(const std::vector<std::uint8_t>& rgba, int width, int height)
{
    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    std::vector<std::uint8_t> output(blocksX * blocksY * 8);

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            // Select a bounding-box endpoint pair for this 4x4 footprint.
            std::array<std::uint8_t, 3> minimum{ 255, 255, 255 };
            std::array<std::uint8_t, 3> maximum{ 0, 0, 0 };
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    const int x = std::min(width - 1, bx * 4 + px);
                    const int y = std::min(height - 1, by * 4 + py);
                    const std::uint8_t* pixel = &rgba[(y * width + x) * 4];
                    for (int channel = 0; channel < 3; ++channel)
                    {
                        minimum[channel] = std::min(minimum[channel], pixel[channel]);
                        maximum[channel] = std::max(maximum[channel], pixel[channel]);
                    }
                }
            }

            std::uint16_t color0 = packRgb565(maximum.data());
            std::uint16_t color1 = packRgb565(minimum.data());
            if (color0 <= color1)
            {
                if (color1 < 0xffff) color0 = static_cast<std::uint16_t>(color1 + 1);
                else color1 = static_cast<std::uint16_t>(color0 - 1);
            }

            // Four-color mode derives two palette entries between the endpoints.
            const auto c0 = unpackRgb565(color0);
            const auto c1 = unpackRgb565(color1);
            std::array<std::array<int, 3>, 4> palette{ c0, c1,
                std::array<int, 3>{ (2 * c0[0] + c1[0]) / 3, (2 * c0[1] + c1[1]) / 3, (2 * c0[2] + c1[2]) / 3 },
                std::array<int, 3>{ (c0[0] + 2 * c1[0]) / 3, (c0[1] + 2 * c1[1]) / 3, (c0[2] + 2 * c1[2]) / 3 }
            };

            // Assign each texel to its least-squares palette match.
            std::uint32_t indices = 0;
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    const int x = std::min(width - 1, bx * 4 + px);
                    const int y = std::min(height - 1, by * 4 + py);
                    const std::uint8_t* pixel = &rgba[(y * width + x) * 4];
                    int best = 0;
                    int bestError = std::numeric_limits<int>::max();
                    for (int candidate = 0; candidate < 4; ++candidate)
                    {
                        int error = 0;
                        for (int channel = 0; channel < 3; ++channel)
                        {
                            const int difference = static_cast<int>(pixel[channel]) - palette[candidate][channel];
                            error += difference * difference;
                        }
                        if (error < bestError)
                        {
                            bestError = error;
                            best = candidate;
                        }
                    }
                    indices |= static_cast<std::uint32_t>(best) << (2 * (py * 4 + px));
                }
            }

            std::uint8_t* block = &output[(by * blocksX + bx) * 8];
            block[0] = static_cast<std::uint8_t>(color0);
            block[1] = static_cast<std::uint8_t>(color0 >> 8);
            block[2] = static_cast<std::uint8_t>(color1);
            block[3] = static_cast<std::uint8_t>(color1 >> 8);
            for (int byte = 0; byte < 4; ++byte)
                block[4 + byte] = static_cast<std::uint8_t>(indices >> (byte * 8));
        }
    }
    return output;
}

// A compact educational ASTC encoder using valid LDR void-extent blocks.
// ASTC stores 128 bits per block; this representation averages each 4x4
// footprint into one RGBA16 constant. Production endpoint/weight searches
// belong in an offline compressor such as astcenc.
std::vector<std::uint8_t> encodeAstc4x4(const std::vector<std::uint8_t>& rgba, int width, int height)
{
    const int blocksX = (width + ASTC_BLOCK - 1) / ASTC_BLOCK;
    const int blocksY = (height + ASTC_BLOCK - 1) / ASTC_BLOCK;
    std::vector<std::uint8_t> output(blocksX * blocksY * 16, 0xff);

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            std::array<unsigned int, 4> sum{};
            for (int py = 0; py < ASTC_BLOCK; ++py)
            {
                for (int px = 0; px < ASTC_BLOCK; ++px)
                {
                    const int x = std::min(width - 1, bx * ASTC_BLOCK + px);
                    const int y = std::min(height - 1, by * ASTC_BLOCK + py);
                    const std::uint8_t* pixel = &rgba[(y * width + x) * 4];
                    for (int channel = 0; channel < 4; ++channel) sum[channel] += pixel[channel];
                }
            }

            // Bits 10..63 are one, leaving the void extent unspecified.
            // Bytes 8..15 contain little-endian RGBA UNORM16 values.
            std::uint8_t* block = &output[(by * blocksX + bx) * 16];
            block[0] = 0xfc; // bits 8..0: ASTC void-extent marker
            block[1] = 0xfd; // LDR, reserved bits and all extent bits set
            for (int channel = 0; channel < 4; ++channel)
            {
                const std::uint16_t value8 = static_cast<std::uint16_t>((sum[channel] + 8) / 16);
                const std::uint16_t value16 = static_cast<std::uint16_t>(value8 * 257);
                block[8 + channel * 2] = static_cast<std::uint8_t>(value16);
                block[9 + channel * 2] = static_cast<std::uint8_t>(value16 >> 8);
            }
        }
    }
    return output;
}

GLuint createRgbaTexture(const std::vector<std::uint8_t>& pixels, int width, int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}

GLuint createCompressedTexture(GLenum format, const std::vector<std::uint8_t>& blocks, int width, int height)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Extension presence is only the first check. The upload and the texture's
    // reported storage state confirm that the driver accepted the format.
    while (glGetError() != GL_NO_ERROR) {}
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                           static_cast<GLsizei>(blocks.size()), blocks.data());
    GLint storedCompressed = GL_FALSE;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &storedCompressed);
    if (glGetError() != GL_NO_ERROR || storedCompressed != GL_TRUE)
    {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

void initializeVirtualTexture(VirtualTexture& virtualTexture)
{
    // The physical cache owns texels. All mip page tables are stacked vertically
    // in one RGBA8 texture; entries store cache-slot XY and residency in blue.
    virtualTexture.tablePixels.assign(VIRTUAL_PAGES * PAGE_TABLE_HEIGHT * 4, 0);
    for (int mip = 0; mip < VIRTUAL_MIP_LEVELS; ++mip)
    {
        const int dimension = mipPageDimension(mip);
        for (int y = 0; y < dimension; ++y)
            for (int x = 0; x < dimension; ++x)
                virtualTexture.pages.push_back({ -1, mip, x, y });
    }

    glGenTextures(1, &virtualTexture.cache);
    glBindTexture(GL_TEXTURE_2D, virtualTexture.cache);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, CACHE_SIZE, CACHE_SIZE, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &virtualTexture.pageTable);
    glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, VIRTUAL_PAGES, PAGE_TABLE_HEIGHT, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, virtualTexture.tablePixels.data());
}

void uploadPage(VirtualTexture& virtualTexture, int pageIndex)
{
    // Prefer a free slot; once full, replace the globally least-recently-used
    // slot and invalidate the evicted page's table entry.
    int selectedSlot = -1;
    std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();
    for (int slotIndex = 0; slotIndex < static_cast<int>(virtualTexture.slots.size()); ++slotIndex)
    {
        if (virtualTexture.slots[slotIndex].page < 0)
        {
            selectedSlot = slotIndex;
            break;
        }
        if (virtualTexture.pages[virtualTexture.slots[slotIndex].page].mip >= PINNED_MIP)
            continue;
        if (virtualTexture.slots[slotIndex].lastUsed < oldest)
        {
            oldest = virtualTexture.slots[slotIndex].lastUsed;
            selectedSlot = slotIndex;
        }
    }
    // The caller limits streaming pages to the non-pinned capacity, so this
    // guard can only trigger if those cache invariants are changed later.
    if (selectedSlot < 0) return;

    VirtualTexture::Slot& slot = virtualTexture.slots[selectedSlot];
    if (slot.page >= 0)
    {
        VirtualTexture::Page& evictedPage = virtualTexture.pages[slot.page];
        evictedPage.slot = -1;
        std::fill_n(&virtualTexture.tablePixels[pageTablePixelOffset(evictedPage)], 4, 0);
    }
    else
    {
        ++virtualTexture.residentCount;
    }

    // Generate the requested page and its border texels on demand. A real
    // streamer would usually obtain this payload from disk or a worker thread.
    VirtualTexture::Page& requestedPage = virtualTexture.pages[pageIndex];
    const int mipScale = 1 << requestedPage.mip;
    std::vector<std::uint8_t> pixels(SLOT_SIZE * SLOT_SIZE * 4);
    for (int y = 0; y < SLOT_SIZE; ++y)
    {
        for (int x = 0; x < SLOT_SIZE; ++x)
        {
            // Sample the center of the source footprint represented by this
            // mip texel. Gutter coordinates naturally fetch adjacent pages.
            const int virtualX = (requestedPage.x * PAGE_SIZE + x - PAGE_GUTTER) * mipScale + mipScale / 2;
            const int virtualY = (requestedPage.y * PAGE_SIZE + y - PAGE_GUTTER) * mipScale + mipScale / 2;
            const auto color = proceduralColor(virtualX, virtualY, VIRTUAL_SIZE);
            std::copy(color.begin(), color.end(), pixels.begin() + (y * SLOT_SIZE + x) * 4);
        }
    }

    // Update only one atlas slot instead of reallocating the physical cache.
    const int slotX = selectedSlot % CACHE_PAGES;
    const int slotY = selectedSlot / CACHE_PAGES;
    glBindTexture(GL_TEXTURE_2D, virtualTexture.cache);
    glTexSubImage2D(GL_TEXTURE_2D, 0, slotX * SLOT_SIZE, slotY * SLOT_SIZE,
                    SLOT_SIZE, SLOT_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Publish the new mapping after its texels have reached the cache.
    slot.page = pageIndex;
    slot.lastUsed = virtualTexture.frame;
    requestedPage.slot = selectedSlot;
    std::uint8_t* entry = &virtualTexture.tablePixels[pageTablePixelOffset(requestedPage)];
    entry[0] = static_cast<std::uint8_t>(slotX);
    entry[1] = static_cast<std::uint8_t>(slotY);
    entry[2] = 255;
    entry[3] = 255;
}

void updateVirtualTexture(VirtualTexture& virtualTexture)
{
    ++virtualTexture.frame;
    virtualTexture.uploadsThisFrame = 0;

    // Keep the 2x2 and 1x1 mip levels permanently resident. Until a detailed
    // page arrives, the shader walks up to one of these complete fallbacks.
    if (virtualTexture.frame == 1)
    {
        for (int mip = PINNED_MIP; mip < VIRTUAL_MIP_LEVELS; ++mip)
        {
            const int dimension = mipPageDimension(mip);
            for (int y = 0; y < dimension; ++y)
                for (int x = 0; x < dimension; ++x)
                    uploadPage(virtualTexture, pageIndex(mip, x, y));
        }
    }

    int pinnedPageCount = 0;
    for (int mip = PINNED_MIP; mip < VIRTUAL_MIP_LEVELS; ++mip)
    {
        const int dimension = mipPageDimension(mip);
        pinnedPageCount += dimension * dimension;
    }
    const int streamingCapacity = static_cast<int>(virtualTexture.slots.size()) - pinnedPageCount;

    // Choose the finest level whose visible rectangle and prefetch border fit
    // in the streaming portion of the cache. This prevents visible-page churn
    // when zooming out instead of blindly requesting hundreds of mip-0 pages.
    virtualTexture.activeMip = PINNED_MIP - 1;
    PageRect requestedRect = requestedPageRect(virtualTexture.activeMip);
    for (int mip = 0; mip < PINNED_MIP; ++mip)
    {
        const PageRect candidate = requestedPageRect(mip);
        if (candidate.count() <= streamingCapacity)
        {
            virtualTexture.activeMip = mip;
            requestedRect = candidate;
            break;
        }
    }

    const int activeDimension = mipPageDimension(virtualTexture.activeMip);
    std::vector<int> requested;
    requested.reserve(static_cast<std::size_t>(requestedRect.count()));
    for (int y = requestedRect.minY; y <= requestedRect.maxY; ++y)
        for (int x = requestedRect.minX; x <= requestedRect.maxX; ++x)
            requested.push_back(pageIndex(virtualTexture.activeMip, x, y));

    // Stream center-first so the most noticeable holes fill first.
    std::sort(requested.begin(), requested.end(), [&](int a, int b)
    {
        const VirtualTexture::Page& pageA = virtualTexture.pages[a];
        const VirtualTexture::Page& pageB = virtualTexture.pages[b];
        const float ax = (pageA.x + 0.5f) / activeDimension - gCenterX;
        const float ay = (pageA.y + 0.5f) / activeDimension - gCenterY;
        const float bx = (pageB.x + 0.5f) / activeDimension - gCenterX;
        const float by = (pageB.y + 0.5f) / activeDimension - gCenterY;
        return ax * ax + ay * ay < bx * bx + by * by;
    });

    // Touch the whole working set before choosing victims. This ensures a new
    // center page cannot evict a still-visible page encountered later below.
    for (int pageIndex : requested)
    {
        VirtualTexture::Page& page = virtualTexture.pages[pageIndex];
        if (page.slot >= 0)
            virtualTexture.slots[page.slot].lastUsed = virtualTexture.frame;
    }

    // Cap misses to avoid a camera jump causing an unbounded upload hitch in
    // one frame. Missing detail is covered by the pinned fallback mip.
    for (int pageIndex : requested)
    {
        VirtualTexture::Page& page = virtualTexture.pages[pageIndex];
        if (page.slot < 0 && virtualTexture.uploadsThisFrame < UPLOADS_PER_FRAME)
        {
            uploadPage(virtualTexture, pageIndex);
            ++virtualTexture.uploadsThisFrame;
        }
    }

    // The complete stacked page table is still only 32x63 texels.
    glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTable);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VIRTUAL_PAGES, PAGE_TABLE_HEIGHT,
                    GL_RGBA, GL_UNSIGNED_BYTE, virtualTexture.tablePixels.data());
}

bool pressedOnce(GLFWwindow* window, int key)
{
    // Edge detection prevents a held key from repeatedly changing modes.
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool result = pressed && !gPreviousKeys[key];
    gPreviousKeys[key] = pressed;
    return result;
}

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (pressedOnce(window, GLFW_KEY_1)) gMode = Mode::Source;
    if (pressedOnce(window, GLFW_KEY_2)) gMode = Mode::Dxt1;
    if (pressedOnce(window, GLFW_KEY_3)) gMode = Mode::Astc;
    if (pressedOnce(window, GLFW_KEY_4)) gMode = Mode::Virtual;
    if (pressedOnce(window, GLFW_KEY_R))
    {
        gCenterX = gCenterY = 0.5f;
        gViewSpan = 0.20f;
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
    gViewSpan = clampFloat(gViewSpan, 0.025f, 1.0f);
    const float halfSpan = gViewSpan * 0.5f;
    gCenterX = clampFloat(gCenterX, halfSpan, 1.0f - halfSpan);
    gCenterY = clampFloat(gCenterY, halfSpan, 1.0f - halfSpan);
}

std::string modeName(Mode mode, bool dxtSupported, bool astcSupported)
{
    switch (mode)
    {
        case Mode::Source: return "RGBA8 source (256 x 256, 256 KiB)";
        case Mode::Dxt1: return dxtSupported ? "DXT1 / BC1 (32 KiB, 8:1)" : "DXT1 unavailable - RGBA8 fallback";
        case Mode::Astc: return astcSupported ? "ASTC 4x4 (64 KiB, 4:1)" : "ASTC unavailable - RGBA8 fallback";
        case Mode::Virtual: return "Virtual texture (2048 x 2048 logical)";
    }
    return {};
}

void updateTitle(GLFWwindow* window, const VirtualTexture& virtualTexture,
                 bool dxtSupported, bool astcSupported)
{
    std::ostringstream title;
    title << "Texture Lab | " << modeName(gMode, dxtSupported, astcSupported);
    if (gMode == Mode::Virtual)
    {
        title << " | cache " << virtualTexture.residentCount << "/" << virtualTexture.slots.size()
              << ", uploads " << virtualTexture.uploadsThisFrame << "/frame"
              << ", mip " << virtualTexture.activeMip
              << ", view " << std::fixed << std::setprecision(1) << gViewSpan * 100.0f << "%";
    }
    title << " | [1-4] mode [WASD/arrows] pan [wheel/Q/E] zoom [R] reset [Esc] quit";
    glfwSetWindowTitle(window, title.str().c_str());
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void scrollCallback(GLFWwindow*, double, double yOffset)
{
    gPendingScroll += yOffset;
}
}

int main()
{
    // Compressed formats are optional even though the sample uses a portable
    // OpenGL 3.3 context, so all compressed allocations are capability-checked.
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Texture Lab", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
    icon(window);

    // Encode the same source into every comparison format. A rejected upload
    // safely aliases the original RGBA8 texture as its fallback.
    const bool dxtExtension = GLAD_GL_EXT_texture_compression_s3tc != 0;
    const bool astcExtension = GLAD_GL_KHR_texture_compression_astc_ldr != 0;
    const std::vector<std::uint8_t> sourcePixels = makeSourceImage();
    const std::vector<std::uint8_t> dxtBlocks = encodeDxt1(sourcePixels, IMAGE_SIZE, IMAGE_SIZE);
    const std::vector<std::uint8_t> astcBlocks = encodeAstc4x4(sourcePixels, IMAGE_SIZE, IMAGE_SIZE);
    const GLuint sourceTexture = createRgbaTexture(sourcePixels, IMAGE_SIZE, IMAGE_SIZE);
    GLuint dxtTexture = dxtExtension
        ? createCompressedTexture(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxtBlocks, IMAGE_SIZE, IMAGE_SIZE)
        : 0;
    GLuint astcTexture = astcExtension
        ? createCompressedTexture(GL_COMPRESSED_RGBA_ASTC_4x4_KHR, astcBlocks, IMAGE_SIZE, IMAGE_SIZE)
        : 0;
    const bool dxtSupported = dxtTexture != 0;
    const bool astcSupported = astcTexture != 0;
    if (!dxtTexture) dxtTexture = sourceTexture;
    if (!astcTexture) astcTexture = sourceTexture;

    const char* dxtStatus = dxtSupported ? "yes"
        : (dxtExtension ? "no (compressed upload rejected; using RGBA8 fallback)"
                        : "no (extension unavailable; using RGBA8 fallback)");
    const char* astcStatus = astcSupported ? "yes"
        : (astcExtension ? "no (compressed upload rejected; using RGBA8 fallback)"
                         : "no (extension unavailable; using RGBA8 fallback)");

    std::cout << "Texture Lab controls:\n"
              << "  1: RGBA8 source  2: DXT1/BC1  3: ASTC 4x4  4: virtual texture\n"
              << "  WASD/arrows: pan  mouse wheel or Q/E: zoom  R: reset  Esc: quit\n\n"
              << "OpenGL renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << '\n'
              << "GPU DXT/S3TC support: " << dxtStatus << '\n'
              << "GPU ASTC LDR support: " << astcStatus << '\n'
              << "Generated payloads: RGBA8=" << sourcePixels.size() << " bytes, DXT1=" << dxtBlocks.size()
              << " bytes, ASTC=" << astcBlocks.size() << " bytes\n";

    // This software-managed indirection does not require a hardware sparse-
    // texture extension, which keeps the residency algorithm visible and portable.
    VirtualTexture virtualTexture;
    initializeVirtualTexture(virtualTexture);

    // A full-screen quad compares the sampling paths without scene distractions.
    const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
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

    Shader shader(FileSystem::getSamplePath("shader/textureShader.vert").c_str(),
                  FileSystem::getSamplePath("shader/textureShader.frag").c_str());
    shader.use();
    shader.setInt("displayTexture", 0);
    shader.setInt("physicalCache", 1);
    shader.setInt("pageTable", 2);

    double previousTime = glfwGetTime();
    double nextTitleUpdate = 0.0;
    // Residency is updated before drawing so a new mapping is visible in the
    // same frame as its cache upload.
    while (!glfwWindowShouldClose(window))
    {
        const double time = glfwGetTime();
        const float deltaTime = static_cast<float>(std::min(0.1, time - previousTime));
        previousTime = time;
        processInput(window, deltaTime);
        if (gMode == Mode::Virtual) updateVirtualTexture(virtualTexture);

        glClearColor(0.025f, 0.03f, 0.045f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        shader.setInt("virtualMode", gMode == Mode::Virtual ? 1 : 0);
        shader.setInt("activeMip", virtualTexture.activeMip);
        glUniform2f(glGetUniformLocation(shader.ID, "viewCenter"), gCenterX, gCenterY);
        shader.setFloat("viewSpan", gViewSpan);

        glActiveTexture(GL_TEXTURE0);
        GLuint displayTexture = sourceTexture;
        if (gMode == Mode::Dxt1) displayTexture = dxtTexture;
        if (gMode == Mode::Astc) displayTexture = astcTexture;
        glBindTexture(GL_TEXTURE_2D, displayTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.cache);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, virtualTexture.pageTable);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (time >= nextTitleUpdate)
        {
            updateTitle(window, virtualTexture, dxtSupported, astcSupported);
            nextTitleUpdate = time + 0.2;
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Fallback textures alias sourceTexture; only successful compressed
    // allocations own a separate object and may be deleted independently.
    if (dxtSupported) glDeleteTextures(1, &dxtTexture);
    if (astcSupported) glDeleteTextures(1, &astcTexture);
    glDeleteTextures(1, &sourceTexture);
    glDeleteTextures(1, &virtualTexture.cache);
    glDeleteTextures(1, &virtualTexture.pageTable);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader.ID);
    glfwTerminate();
    return 0;
}
