#pragma once

#include <glad/glad.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class VirtualTexture
{
public:
    static constexpr int CachePages = 11;
    static constexpr int SlotSize = 66;
    static constexpr int CacheSize = CachePages * SlotSize;

    void initialize();
    void update(float centerX, float centerY, float viewSpan);
    void destroy();

    GLuint cacheTexture() const { return cache_; }
    GLuint pageTableTexture() const { return pageTable_; }
    int residentCount() const { return residentCount_; }
    int slotCount() const { return static_cast<int>(slots_.size()); }
    int uploadsThisFrame() const { return uploadsThisFrame_; }
    int activeMip() const { return activeMip_; }

private:
    struct Page
    {
        int slot = -1;
        int mip = 0;
        int x = 0;
        int y = 0;
    };

    struct Slot
    {
        int page = -1;
        std::uint64_t lastUsed = 0;
    };

    struct PageRect
    {
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        int count() const { return (maxX - minX + 1) * (maxY - minY + 1); }
    };

    static int mipDimension(int mip);
    static int tableYOffset(int mip);
    static int mipPageOffset(int mip);
    static int pageIndex(int mip, int x, int y);
    static PageRect requestedRect(int mip, float centerX, float centerY, float viewSpan);
    std::size_t tablePixelOffset(const Page& page) const;
    void uploadPage(int pageIndex);

    GLuint cache_ = 0;
    GLuint pageTable_ = 0;
    std::vector<Page> pages_;
    std::array<Slot, CachePages * CachePages> slots_{};
    std::vector<std::uint8_t> tablePixels_;
    std::uint64_t frame_ = 0;
    int residentCount_ = 0;
    int uploadsThisFrame_ = 0;
    int activeMip_ = 0;
};
