#include "virtual_texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
constexpr int VirtualSize = 2048;
constexpr int PageSize = 64;
constexpr int VirtualPages = VirtualSize / PageSize;
constexpr int MipLevels = 6;
constexpr int PinnedMip = 4;
constexpr int PageTableHeight = 63;
constexpr int PageGutter = 1;
constexpr int UploadsPerFrame = 4;

std::uint8_t toByte(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return static_cast<std::uint8_t>(value * 255.0f + 0.5f);
}

// Procedural texels keep the residency example independent of disk I/O.
std::array<std::uint8_t, 4> virtualColor(int x, int y)
{
    x = std::max(0, std::min(VirtualSize - 1, x));
    y = std::max(0, std::min(VirtualSize - 1, y));
    const float u = static_cast<float>(x) / static_cast<float>(VirtualSize - 1);
    const float v = static_cast<float>(y) / static_cast<float>(VirtualSize - 1);
    const float checker = (((x / 128) + (y / 128)) & 1) ? 0.14f : 0.0f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float rings = 0.5f + 0.5f * std::sin(std::sqrt(dx * dx + dy * dy) * 95.0f);
    const bool grid = x % 256 < 8 || y % 256 < 8;
    if (grid) return { 245, 245, 245, 255 };
    return { toByte(0.10f + 0.70f * u + checker),
             toByte(0.10f + 0.70f * v + checker),
             toByte(0.16f + 0.34f * rings + checker), 255 };
}
}

int VirtualTexture::mipDimension(int mip)
{
    return std::max(1, VirtualPages >> mip);
}

int VirtualTexture::tableYOffset(int mip)
{
    int offset = 0;
    for (int level = 0; level < mip; ++level) offset += mipDimension(level);
    return offset;
}

int VirtualTexture::mipPageOffset(int mip)
{
    int offset = 0;
    for (int level = 0; level < mip; ++level)
    {
        const int dimension = mipDimension(level);
        offset += dimension * dimension;
    }
    return offset;
}

int VirtualTexture::pageIndex(int mip, int x, int y)
{
    return mipPageOffset(mip) + y * mipDimension(mip) + x;
}

VirtualTexture::PageRect VirtualTexture::requestedRect(int mip, float centerX,
                                                       float centerY, float viewSpan)
{
    const int dimension = mipDimension(mip);
    const float halfSpan = viewSpan * 0.5f;
    return {
        std::max(0, static_cast<int>(std::floor((centerX - halfSpan) * dimension)) - 1),
        std::min(dimension - 1, static_cast<int>(std::floor((centerX + halfSpan) * dimension)) + 1),
        std::max(0, static_cast<int>(std::floor((centerY - halfSpan) * dimension)) - 1),
        std::min(dimension - 1, static_cast<int>(std::floor((centerY + halfSpan) * dimension)) + 1)
    };
}

std::size_t VirtualTexture::tablePixelOffset(const Page& page) const
{
    return static_cast<std::size_t>(((tableYOffset(page.mip) + page.y) *
                                    VirtualPages + page.x) * 4);
}

void VirtualTexture::initialize()
{
    tablePixels_.assign(VirtualPages * PageTableHeight * 4, 0);
    for (int mip = 0; mip < MipLevels; ++mip)
    {
        const int dimension = mipDimension(mip);
        for (int y = 0; y < dimension; ++y)
            for (int x = 0; x < dimension; ++x)
                pages_.push_back({ -1, mip, x, y });
    }

    glGenTextures(1, &cache_);
    glBindTexture(GL_TEXTURE_2D, cache_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, CacheSize, CacheSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &pageTable_);
    glBindTexture(GL_TEXTURE_2D, pageTable_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, VirtualPages, PageTableHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, tablePixels_.data());
}

void VirtualTexture::uploadPage(int requestedIndex)
{
    int selectedSlot = -1;
    std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();
    for (int index = 0; index < static_cast<int>(slots_.size()); ++index)
    {
        if (slots_[index].page < 0)
        {
            selectedSlot = index;
            break;
        }
        if (pages_[slots_[index].page].mip >= PinnedMip) continue;
        if (slots_[index].lastUsed < oldest)
        {
            oldest = slots_[index].lastUsed;
            selectedSlot = index;
        }
    }
    if (selectedSlot < 0) return;

    Slot& slot = slots_[selectedSlot];
    if (slot.page >= 0)
    {
        Page& evicted = pages_[slot.page];
        evicted.slot = -1;
        std::fill_n(&tablePixels_[tablePixelOffset(evicted)], 4, 0);
    }
    else
    {
        ++residentCount_;
    }

    Page& requested = pages_[requestedIndex];
    const int mipScale = 1 << requested.mip;
    std::vector<std::uint8_t> pixels(SlotSize * SlotSize * 4);
    for (int y = 0; y < SlotSize; ++y)
        for (int x = 0; x < SlotSize; ++x)
        {
            const int virtualX = (requested.x * PageSize + x - PageGutter) * mipScale + mipScale / 2;
            const int virtualY = (requested.y * PageSize + y - PageGutter) * mipScale + mipScale / 2;
            const auto color = virtualColor(virtualX, virtualY);
            std::copy(color.begin(), color.end(), pixels.begin() + (y * SlotSize + x) * 4);
        }

    const int slotX = selectedSlot % CachePages;
    const int slotY = selectedSlot / CachePages;
    glBindTexture(GL_TEXTURE_2D, cache_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, slotX * SlotSize, slotY * SlotSize,
                    SlotSize, SlotSize, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    slot.page = requestedIndex;
    slot.lastUsed = frame_;
    requested.slot = selectedSlot;
    std::uint8_t* entry = &tablePixels_[tablePixelOffset(requested)];
    entry[0] = static_cast<std::uint8_t>(slotX);
    entry[1] = static_cast<std::uint8_t>(slotY);
    entry[2] = entry[3] = 255;
}

void VirtualTexture::update(float centerX, float centerY, float viewSpan)
{
    ++frame_;
    uploadsThisFrame_ = 0;

    // These five pages guarantee a complete fallback image from frame one.
    if (frame_ == 1)
        for (int mip = PinnedMip; mip < MipLevels; ++mip)
        {
            const int dimension = mipDimension(mip);
            for (int y = 0; y < dimension; ++y)
                for (int x = 0; x < dimension; ++x)
                    uploadPage(pageIndex(mip, x, y));
        }

    constexpr int pinnedPages = 5; // 2x2 plus 1x1
    const int streamingCapacity = slotCount() - pinnedPages;
    activeMip_ = PinnedMip - 1;
    PageRect rectangle = requestedRect(activeMip_, centerX, centerY, viewSpan);
    for (int mip = 0; mip < PinnedMip; ++mip)
    {
        const PageRect candidate = requestedRect(mip, centerX, centerY, viewSpan);
        if (candidate.count() <= streamingCapacity)
        {
            activeMip_ = mip;
            rectangle = candidate;
            break;
        }
    }

    std::vector<int> requested;
    requested.reserve(static_cast<std::size_t>(rectangle.count()));
    for (int y = rectangle.minY; y <= rectangle.maxY; ++y)
        for (int x = rectangle.minX; x <= rectangle.maxX; ++x)
            requested.push_back(pageIndex(activeMip_, x, y));

    const int dimension = mipDimension(activeMip_);
    std::sort(requested.begin(), requested.end(), [&](int a, int b)
    {
        const Page& pageA = pages_[a];
        const Page& pageB = pages_[b];
        const float ax = (pageA.x + 0.5f) / dimension - centerX;
        const float ay = (pageA.y + 0.5f) / dimension - centerY;
        const float bx = (pageB.x + 0.5f) / dimension - centerX;
        const float by = (pageB.y + 0.5f) / dimension - centerY;
        return ax * ax + ay * ay < bx * bx + by * by;
    });

    // Protect the entire visible set before the LRU pass selects victims.
    for (int index : requested)
        if (pages_[index].slot >= 0)
            slots_[pages_[index].slot].lastUsed = frame_;
    for (int index : requested)
        if (pages_[index].slot < 0 && uploadsThisFrame_ < UploadsPerFrame)
        {
            uploadPage(index);
            ++uploadsThisFrame_;
        }

    glBindTexture(GL_TEXTURE_2D, pageTable_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VirtualPages, PageTableHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, tablePixels_.data());
}

void VirtualTexture::destroy()
{
    glDeleteTextures(1, &cache_);
    glDeleteTextures(1, &pageTable_);
    cache_ = pageTable_ = 0;
}
