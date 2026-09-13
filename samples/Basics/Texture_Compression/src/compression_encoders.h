#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace compression
{
constexpr int BLOCK_SIZE = 4;

inline std::uint8_t toByte(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return static_cast<std::uint8_t>(value * 255.0f + 0.5f);
}

// A grid, checker, gradient, and rings expose block-compression artifacts.
inline std::array<std::uint8_t, 4> testColor(int x, int y, int size)
{
    x = std::max(0, std::min(size - 1, x));
    y = std::max(0, std::min(size - 1, y));
    const float u = static_cast<float>(x) / static_cast<float>(size - 1);
    const float v = static_cast<float>(y) / static_cast<float>(size - 1);
    const float checker = (((x / (size / 16)) + (y / (size / 16))) & 1) ? 0.14f : 0.0f;
    const float dx = u - 0.5f;
    const float dy = v - 0.5f;
    const float rings = 0.5f + 0.5f * std::sin(std::sqrt(dx * dx + dy * dy) * 95.0f);
    const bool grid = x % (size / 8) < std::max(1, size / 256) ||
                      y % (size / 8) < std::max(1, size / 256);
    if (grid) return { 245, 245, 245, 255 };
    return { toByte(0.10f + 0.70f * u + checker),
             toByte(0.10f + 0.70f * v + checker),
             toByte(0.16f + 0.34f * rings + checker), 255 };
}

inline std::vector<std::uint8_t> makeTestImage(int size)
{
    std::vector<std::uint8_t> pixels(size * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const auto color = testColor(x, y, size);
            std::copy(color.begin(), color.end(), pixels.begin() + (y * size + x) * 4);
        }
    return pixels;
}

inline std::uint16_t packRgb565(const std::uint8_t* color)
{
    return static_cast<std::uint16_t>(((color[0] >> 3) << 11) |
                                      ((color[1] >> 2) << 5) |
                                      (color[2] >> 3));
}

inline std::array<int, 3> unpackRgb565(std::uint16_t color)
{
    return { static_cast<int>(((color >> 11) & 31) * 255 / 31),
             static_cast<int>(((color >> 5) & 63) * 255 / 63),
             static_cast<int>((color & 31) * 255 / 31) };
}

// DXT1 stores two RGB565 endpoints and sixteen two-bit palette indices in each
// 64-bit block. This compact encoder uses the block's RGB bounding box.
inline std::vector<std::uint8_t> encodeDxt1(const std::vector<std::uint8_t>& rgba,
                                             int width, int height)
{
    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    std::vector<std::uint8_t> output(blocksX * blocksY * 8);

    for (int by = 0; by < blocksY; ++by)
    {
        for (int bx = 0; bx < blocksX; ++bx)
        {
            std::array<std::uint8_t, 3> minimum{ 255, 255, 255 };
            std::array<std::uint8_t, 3> maximum{ 0, 0, 0 };
            for (int py = 0; py < 4; ++py)
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

            std::uint16_t color0 = packRgb565(maximum.data());
            std::uint16_t color1 = packRgb565(minimum.data());
            if (color0 <= color1)
            {
                if (color1 < 0xffff) color0 = static_cast<std::uint16_t>(color1 + 1);
                else color1 = static_cast<std::uint16_t>(color0 - 1);
            }
            const auto c0 = unpackRgb565(color0);
            const auto c1 = unpackRgb565(color1);
            const std::array<std::array<int, 3>, 4> palette{ c0, c1,
                std::array<int, 3>{ (2 * c0[0] + c1[0]) / 3, (2 * c0[1] + c1[1]) / 3, (2 * c0[2] + c1[2]) / 3 },
                std::array<int, 3>{ (c0[0] + 2 * c1[0]) / 3, (c0[1] + 2 * c1[1]) / 3, (c0[2] + 2 * c1[2]) / 3 }
            };

            std::uint32_t indices = 0;
            for (int py = 0; py < 4; ++py)
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
                            best = candidate;
                            bestError = error;
                        }
                    }
                    indices |= static_cast<std::uint32_t>(best) << (2 * (py * 4 + px));
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

// A valid LDR void-extent block stores one averaged RGBA16 color. It is useful
// for demonstrating ASTC storage, while production encoders search endpoints
// and weights to retain detail within each block.
inline std::vector<std::uint8_t> encodeAstc4x4(const std::vector<std::uint8_t>& rgba,
                                               int width, int height)
{
    const int blocksX = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    const int blocksY = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    std::vector<std::uint8_t> output(blocksX * blocksY * 16, 0xff);
    for (int by = 0; by < blocksY; ++by)
        for (int bx = 0; bx < blocksX; ++bx)
        {
            std::array<unsigned int, 4> sum{};
            for (int py = 0; py < BLOCK_SIZE; ++py)
                for (int px = 0; px < BLOCK_SIZE; ++px)
                {
                    const int x = std::min(width - 1, bx * BLOCK_SIZE + px);
                    const int y = std::min(height - 1, by * BLOCK_SIZE + py);
                    const std::uint8_t* pixel = &rgba[(y * width + x) * 4];
                    for (int channel = 0; channel < 4; ++channel) sum[channel] += pixel[channel];
                }

            std::uint8_t* block = &output[(by * blocksX + bx) * 16];
            block[0] = 0xfc;
            block[1] = 0xfd;
            for (int channel = 0; channel < 4; ++channel)
            {
                const std::uint16_t value8 = static_cast<std::uint16_t>((sum[channel] + 8) / 16);
                const std::uint16_t value16 = static_cast<std::uint16_t>(value8 * 257);
                block[8 + channel * 2] = static_cast<std::uint8_t>(value16);
                block[9 + channel * 2] = static_cast<std::uint8_t>(value16 >> 8);
            }
        }
    return output;
}
}
