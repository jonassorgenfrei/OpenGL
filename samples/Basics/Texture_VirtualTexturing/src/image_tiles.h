#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Disk source, separate from the GPU page table/cache. Only the small index and
// four decoded tiles stay in RAM; no full-resolution image is loaded at runtime.
class ImageTileSource
{
public:
    void open(const std::string& path, int virtualSize, int pageSize, int mipLevels);
    std::vector<std::uint8_t> readPage(int mip, int pageX, int pageY);
    void close();
    int diskReads() const { return diskReads_; }

private:
    static constexpr int TileSize = 256;
    static constexpr int Gutter = 16;
    struct Record
    {
        std::uint64_t offset;
        std::uint32_t length;
    };
    struct DecodedTile
    {
        int index = -1;
        int width = 0;
        std::uint64_t lastUsed = 0;
        std::vector<std::uint8_t> pixels;
    };
    std::ifstream file_;
    std::vector<Record> records_;
    std::array<DecodedTile, 4> decoded_{};
    int virtualSize_ = 0;
    int pageSize_ = 0;
    int diskReads_ = 0;
    std::uint64_t clock_ = 0;
};
