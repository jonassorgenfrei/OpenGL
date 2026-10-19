#pragma once
#include <glad/glad.h>
#include "image_tiles.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace virtual_texture
{
constexpr int VirtualSize = 16384;
constexpr int PageSize = 64;
constexpr int VirtualPages = VirtualSize / PageSize;
constexpr int MipLevels = 9;
constexpr int PinnedMip = MipLevels - 2;
constexpr int PageTableHeight = VirtualPages * 2 - 1;
constexpr int CachePages = 11;
constexpr int SlotSize = PageSize + 2;
constexpr int CacheSize = CachePages * SlotSize;
static_assert(CachePages * CachePages >= 5, "Cache must hold the five pinned fallback pages");
static_assert(CachePages <= 256, "Page-table coordinates use one byte per axis");

struct Page
{
    int slot = -1;
    int mip = 0;
    int x = 0;
    int y = 0;
    std::size_t tableOffset = 0;
};
struct Slot
{
    int page = -1;
    std::uint64_t lastUsed = 0;
};
// Plain OpenGL state: initialization -> page requests/uploads -> drawing -> cleanup.
struct State
{
    image_tiles::State source;
    GLuint cache = 0;
    GLuint pageTable = 0;
    std::vector<Page> pages;
    std::array<Slot, CachePages * CachePages> slots{};
    std::array<int, MipLevels> mipOffsets{};
    std::vector<std::uint8_t> tablePixels;
    std::uint64_t frame = 0;
    int residentCount = 0;
    int uploadsThisFrame = 0;
    int activeMip = 0;
    int pendingCount = 0;
    int evictions = 0;
    bool tableChanged = false;
};
void initialize(State &texture, const std::string &imagePath);
void update(State &texture, float centerX, float centerY, float viewSpan, bool stream = true, int uploadBudget = 4);
void clearDetail(State &texture);
void destroy(State &texture);
} // namespace virtual_texture
