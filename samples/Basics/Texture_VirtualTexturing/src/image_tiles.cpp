#include "image_tiles.h"

#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace
{
std::uint64_t readLittleEndian(std::istream& stream, int byteCount)
{
    std::uint64_t value = 0;
    for (int byte = 0; byte < byteCount; ++byte)
    {
        const int next = stream.get();
        if (next == std::char_traits<char>::eof())
            throw std::runtime_error("Truncated virtual-texture tile index");
        value |= static_cast<std::uint64_t>(next) << (byte * 8);
    }
    return value;
}
}

void ImageTileSource::open(const std::string& path, int virtualSize, int pageSize, int mipLevels)
{
    close();
    file_.open(path, std::ios::binary);
    if (!file_) throw std::runtime_error("Cannot open virtual-texture image: " + path);
    file_.seekg(0, std::ios::end);
    const auto fileSize = static_cast<std::uint64_t>(file_.tellg());
    file_.seekg(0);
    char magic[8] = {};
    file_.read(magic, sizeof(magic));
    if (std::memcmp(magic, "VTPACK01", sizeof(magic)) != 0 ||
        readLittleEndian(file_, 4) != static_cast<std::uint64_t>(virtualSize) ||
        readLittleEndian(file_, 4) != static_cast<std::uint64_t>(pageSize) ||
        readLittleEndian(file_, 4) != static_cast<std::uint64_t>(mipLevels) ||
        readLittleEndian(file_, 4) != TileSize || readLittleEndian(file_, 4) != Gutter)
        throw std::runtime_error("Unsupported virtual-texture image header: " + path);
    int expectedCount = 0;
    for (int mip = 0; mip < mipLevels; ++mip)
    {
        const int dimension = std::max(1, (virtualSize >> mip) / TileSize);
        expectedCount += dimension * dimension;
    }
    if (readLittleEndian(file_, 4) != static_cast<std::uint64_t>(expectedCount))
        throw std::runtime_error("Invalid virtual-texture image tile count");
    const std::uint64_t indexEnd = 32 + expectedCount * 12;
    records_.reserve(expectedCount);
    for (int tile = 0; tile < expectedCount; ++tile)
    {
        Record record{readLittleEndian(file_, 8), static_cast<std::uint32_t>(readLittleEndian(file_, 4))};
        if (record.offset < indexEnd || record.offset > fileSize || record.length == 0 ||
            record.length > 1024 * 1024 || record.length > fileSize - record.offset)
            throw std::runtime_error("Invalid virtual-texture image tile bounds");
        records_.push_back(record);
    }
    virtualSize_ = virtualSize;
    pageSize_ = pageSize;
}

std::vector<std::uint8_t> ImageTileSource::readPage(int mip, int pageX, int pageY)
{
    const int imageSize = virtualSize_ >> mip;
    const int dimension = std::max(1, imageSize / TileSize);
    const int pixelX = pageX * pageSize_;
    const int pixelY = pageY * pageSize_;
    int index = (pixelY / TileSize) * dimension + pixelX / TileSize;
    for (int level = 0; level < mip; ++level)
    {
        const int count = std::max(1, (virtualSize_ >> level) / TileSize);
        index += count * count;
    }
    DecodedTile* tile = nullptr;
    for (auto& candidate : decoded_)
        if (candidate.index == index) { tile = &candidate; break; }
    if (!tile)
    {
        tile = &*std::min_element(decoded_.begin(), decoded_.end(),
            [](const DecodedTile& a, const DecodedTile& b) { return a.lastUsed < b.lastUsed; });
        const Record& record = records_.at(index);
        std::vector<std::uint8_t> compressed(record.length);
        file_.clear();
        file_.seekg(static_cast<std::streamoff>(record.offset));
        if (!file_.read(reinterpret_cast<char*>(compressed.data()), record.length))
            throw std::runtime_error("Cannot read virtual-texture image tile");
        int width, height, channels;
        const int expectedSize = std::min(TileSize, imageSize) + 2 * Gutter;
        // Validate dimensions before allowing the decoder to allocate storage.
        if (!stbi_info_from_memory(compressed.data(), static_cast<int>(compressed.size()), &width, &height, &channels) ||
            width != expectedSize || height != expectedSize)
            throw std::runtime_error("Invalid virtual-texture image tile dimensions");
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
            stbi_load_from_memory(compressed.data(), static_cast<int>(compressed.size()), &width, &height, &channels, 4),
            stbi_image_free);
        if (!pixels) throw std::runtime_error("Cannot decode virtual-texture image tile");
        tile->pixels.assign(pixels.get(), pixels.get() + width * height * 4);
        tile->width = width;
        tile->index = index;
        ++diskReads_;
    }
    tile->lastUsed = ++clock_;
    const int slotSize = pageSize_ + 2;
    std::vector<std::uint8_t> page(slotSize * slotSize * 4);
    // The disk tile's neighboring texels supply the GPU page's one-pixel gutter.
    // Rows in the pack already run south-to-north to match OpenGL texture UVs.
    for (int y = 0; y < slotSize; ++y)
    {
        const int sourceX = pixelX % TileSize + Gutter - 1;
        const int sourceY = pixelY % TileSize + Gutter - 1 + y;
        std::copy_n(&tile->pixels[(sourceY * tile->width + sourceX) * 4], slotSize * 4,
                    &page[y * slotSize * 4]);
    }
    return page;
}

void ImageTileSource::close()
{
    file_.close();
    file_.clear();
    records_.clear();
    for (auto& tile : decoded_) tile = DecodedTile{};
    diskReads_ = 0;
    clock_ = 0;
}
