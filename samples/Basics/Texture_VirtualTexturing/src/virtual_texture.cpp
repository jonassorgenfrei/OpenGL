#include "virtual_texture.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace virtual_texture
{
// Uploading a page is the one place that updates both residency and its table entry.
static void uploadPage(State &texture, int requestedIndex);

void initialize(State &texture, const std::string &imagePath)
{
    destroy(texture);
    image_tiles::open(texture.source, imagePath, VirtualSize, PageSize, MipLevels);
    texture.tablePixels.assign(VirtualPages * PageTableHeight * 4, 0);
    int tableY = 0;
    for (int mip = 0; mip < MipLevels; ++mip)
    {
        const int dimension = VirtualPages >> mip;
        texture.mipOffsets[mip] = static_cast<int>(texture.pages.size());
        for (int y = 0; y < dimension; ++y)
            for (int x = 0; x < dimension; ++x)
                texture.pages.push_back(
                    {-1, mip, x, y, static_cast<std::size_t>(((tableY + y) * VirtualPages + x) * 4)});
        tableY += dimension;
    }

    glGenTextures(1, &texture.cache);
    glBindTexture(GL_TEXTURE_2D, texture.cache);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const std::vector<std::uint8_t> emptyCache(CacheSize * CacheSize * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, CacheSize, CacheSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, emptyCache.data());

    glGenTextures(1, &texture.pageTable);
    glBindTexture(GL_TEXTURE_2D, texture.pageTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, VirtualPages, PageTableHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 texture.tablePixels.data());
}

static void uploadPage(State &texture, int requestedIndex)
{
    int selectedSlot = -1;
    std::uint64_t oldest = std::numeric_limits<std::uint64_t>::max();
    for (int index = 0; index < static_cast<int>(texture.slots.size()); ++index)
    {
        if (texture.slots[index].page < 0)
        {
            selectedSlot = index;
            break;
        }
        if (texture.pages[texture.slots[index].page].mip >= PinnedMip)
            continue;
        if (texture.slots[index].lastUsed < oldest)
        {
            oldest = texture.slots[index].lastUsed;
            selectedSlot = index;
        }
    }
    if (selectedSlot < 0)
        return;

    Slot &slot = texture.slots[selectedSlot];
    if (slot.page >= 0)
    {
        Page &evicted = texture.pages[slot.page];
        ++texture.evictions;
        evicted.slot = -1;
        std::fill_n(&texture.tablePixels[evicted.tableOffset], 4, 0);
    }
    else
    {
        ++texture.residentCount;
    }

    Page &requested = texture.pages[requestedIndex];
    const auto pixels = image_tiles::readPage(texture.source, requested.mip, requested.x, requested.y);

    const int slotX = selectedSlot % CachePages;
    const int slotY = selectedSlot / CachePages;
    glBindTexture(GL_TEXTURE_2D, texture.cache);
    glTexSubImage2D(GL_TEXTURE_2D, 0, slotX * SlotSize, slotY * SlotSize, SlotSize, SlotSize, GL_RGBA, GL_UNSIGNED_BYTE,
                    pixels.data());

    slot.page = requestedIndex;
    slot.lastUsed = texture.frame;
    requested.slot = selectedSlot;
    std::uint8_t *entry = &texture.tablePixels[requested.tableOffset];
    entry[0] = static_cast<std::uint8_t>(slotX);
    entry[1] = static_cast<std::uint8_t>(slotY);
    entry[2] = entry[3] = 255;
    texture.tableChanged = true;
}

void update(State &texture, float centerX, float centerY, float viewSpan, bool stream, int uploadBudget)
{
    ++texture.frame;
    texture.uploadsThisFrame = 0;

    // Bootstrap the five fallback pages, outside the detailed upload budget.
    if (texture.frame == 1)
        for (int page = texture.mipOffsets[PinnedMip]; page < static_cast<int>(texture.pages.size()); ++page)
            uploadPage(texture, page);

    const int streamingCapacity = static_cast<int>(texture.slots.size()) - 5;
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    const float halfSpan = viewSpan * 0.5f;
    // Choose the finest requested rectangle that fits. Tiny caches can always
    // use the already pinned mip rather than continuously evicting visible pages.
    for (int mip = 0; mip <= PinnedMip; ++mip)
    {
        const int dimension = VirtualPages >> mip;
        minX = std::max(0, static_cast<int>(std::floor((centerX - halfSpan) * dimension)) - 1);
        maxX = std::min(dimension - 1, static_cast<int>(std::floor((centerX + halfSpan) * dimension)) + 1);
        minY = std::max(0, static_cast<int>(std::floor((centerY - halfSpan) * dimension)) - 1);
        maxY = std::min(dimension - 1, static_cast<int>(std::floor((centerY + halfSpan) * dimension)) + 1);
        texture.activeMip = mip;
        if ((maxX - minX + 1) * (maxY - minY + 1) <= streamingCapacity || mip == PinnedMip)
            break;
    }

    std::vector<int> requested;
    requested.reserve(static_cast<std::size_t>((maxX - minX + 1) * (maxY - minY + 1)));
    for (int y = minY; y <= maxY; ++y)
        for (int x = minX; x <= maxX; ++x)
            requested.push_back(texture.mipOffsets[texture.activeMip] + y * (VirtualPages >> texture.activeMip) + x);

    const int dimension = (VirtualPages >> texture.activeMip);
    std::sort(requested.begin(), requested.end(),
              [&](int a, int b)
              {
                  const Page &pageA = texture.pages[a];
                  const Page &pageB = texture.pages[b];
                  const float ax = (pageA.x + 0.5f) / dimension - centerX;
                  const float ay = (pageA.y + 0.5f) / dimension - centerY;
                  const float bx = (pageB.x + 0.5f) / dimension - centerX;
                  const float by = (pageB.y + 0.5f) / dimension - centerY;
                  return ax * ax + ay * ay < bx * bx + by * by;
              });

    // Protect the entire visible set before the LRU pass selects victims.
    for (int index : requested)
        if (texture.pages[index].slot >= 0)
            texture.slots[texture.pages[index].slot].lastUsed = texture.frame;
    for (int index : requested)
        if (texture.pages[index].slot < 0 && stream && texture.uploadsThisFrame < uploadBudget)
        {
            uploadPage(texture, index);
            ++texture.uploadsThisFrame;
        }

    texture.pendingCount = 0;
    for (int index : requested)
        if (texture.pages[index].slot < 0)
            ++texture.pendingCount;

    if (texture.tableChanged)
    {
        glBindTexture(GL_TEXTURE_2D, texture.pageTable);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VirtualPages, PageTableHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                        texture.tablePixels.data());
        texture.tableChanged = false;
    }
}

// Keep the five coarse fallback pages, but force fine detail to stream again.
void clearDetail(State &texture)
{
    // Clear discarded texels too, so the cache preview agrees with residency.
    const std::vector<std::uint8_t> emptySlot(SlotSize * SlotSize * 4, 0);
    glBindTexture(GL_TEXTURE_2D, texture.cache);
    for (int index = 0; index < static_cast<int>(texture.slots.size()); ++index)
    {
        Slot &slot = texture.slots[index];
        if (slot.page < 0 || texture.pages[slot.page].mip >= PinnedMip)
            continue;
        glTexSubImage2D(GL_TEXTURE_2D, 0, index % CachePages * SlotSize, index / CachePages * SlotSize, SlotSize,
                        SlotSize, GL_RGBA, GL_UNSIGNED_BYTE, emptySlot.data());
        Page &page = texture.pages[slot.page];
        std::fill_n(&texture.tablePixels[page.tableOffset], 4, 0);
        page.slot = -1;
        slot = Slot{};
        --texture.residentCount;
        texture.tableChanged = true;
    }
}

void destroy(State &texture)
{
    image_tiles::close(texture.source);
    glDeleteTextures(1, &texture.cache);
    glDeleteTextures(1, &texture.pageTable);
    texture = State{};
}

} // namespace virtual_texture
