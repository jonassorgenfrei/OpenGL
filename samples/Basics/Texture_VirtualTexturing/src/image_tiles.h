#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// File reads and JPEG decoding stay separate from the OpenGL residency lesson.
namespace image_tiles
{
constexpr int TileSize = 256;
constexpr int Gutter = 16;
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
struct State
{
    std::ifstream file;
    std::vector<Record> records;
    std::array<DecodedTile, 4> decoded{};
    int virtualSize = 0;
    int pageSize = 0;
    int mipLevels = 0;
    int diskReads = 0;
    std::uint64_t clock = 0;
};
void open(State &source, const std::string &path, int virtualSize, int pageSize, int mipLevels);
std::vector<std::uint8_t> readPage(State &source, int mip, int pageX, int pageY);
void close(State &source);
} // namespace image_tiles
