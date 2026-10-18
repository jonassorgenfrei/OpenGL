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

// BC2/DXT3 and BC3/DXT5 reuse the opaque four-color BC1 RGB block.
// BC2 stores sixteen explicit 4-bit alpha values. BC3 instead stores two
// 8-bit endpoints and sixteen 3-bit indices into an interpolated alpha palette.
inline std::vector<std::uint8_t> encodeDxtAlpha(const std::vector<std::uint8_t>& rgba,
                                               int width, int height, bool interpolated)
{
    const auto rgb = encodeDxt1(rgba, width, height);
    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    std::vector<std::uint8_t> output(blocksX * blocksY * 16);
    for (int by = 0; by < blocksY; ++by)
        for (int bx = 0; bx < blocksX; ++bx)
        {
            const int blockIndex = by * blocksX + bx;
            auto* block = &output[blockIndex * 16];
            std::copy_n(&rgb[blockIndex * 8], 8, block + 8);
            std::array<int, 16> alpha{};
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 4; ++x)
                    alpha[y * 4 + x] = rgba[(std::min(height - 1, by * 4 + y) * width +
                                             std::min(width - 1, bx * 4 + x)) * 4 + 3];
            if (!interpolated)
            {
                for (int texel = 0; texel < 16; ++texel)
                    block[texel / 2] |= static_cast<std::uint8_t>(
                        ((alpha[texel] * 15 + 127) / 255) << ((texel % 2) * 4));
                continue;
            }
            const int minimum = *std::min_element(alpha.begin(), alpha.end());
            const int maximum = *std::max_element(alpha.begin(), alpha.end());
            int bestError = std::numeric_limits<int>::max();
            // Try the eight-value mode and the six-value mode with explicit
            // zero/one entries. Endpoints are kept simple for readable code.
            for (int mode = 0; mode < 2; ++mode)
            {
                const int a0 = mode == 0 ? maximum : minimum;
                const int a1 = mode == 0 ? minimum : maximum;
                std::array<int, 8> palette{ a0, a1 };
                if (a0 > a1)
                    for (int i = 1; i <= 6; ++i) palette[i + 1] = ((7 - i) * a0 + i * a1) / 7;
                else
                {
                    for (int i = 1; i <= 4; ++i) palette[i + 1] = ((5 - i) * a0 + i * a1) / 5;
                    palette[6] = 0; palette[7] = 255;
                }
                int error = 0;
                std::uint64_t indices = 0;
                for (int texel = 0; texel < 16; ++texel)
                {
                    int best = 0;
                    for (int candidate = 1; candidate < 8; ++candidate)
                        if (std::abs(alpha[texel] - palette[candidate]) < std::abs(alpha[texel] - palette[best]))
                            best = candidate;
                    const int difference = alpha[texel] - palette[best];
                    error += difference * difference;
                    indices |= static_cast<std::uint64_t>(best) << (3 * texel);
                }
                if (error < bestError)
                {
                    bestError = error;
                    block[0] = static_cast<std::uint8_t>(a0);
                    block[1] = static_cast<std::uint8_t>(a1);
                    for (int byte = 0; byte < 6; ++byte)
                        block[2 + byte] = static_cast<std::uint8_t>(indices >> (8 * byte));
                }
            }
        }
    return output;
}

inline std::vector<std::uint8_t> encodeDxt3(const std::vector<std::uint8_t>& rgba, int width, int height)
{
    return encodeDxtAlpha(rgba, width, height, false);
}

inline std::vector<std::uint8_t> encodeDxt5(const std::vector<std::uint8_t>& rgba, int width, int height)
{
    return encodeDxtAlpha(rgba, width, height, true);
}
// CPU preview for machines without S3TC. These bytes are uploaded as RGBA8;
// that fallback is never counted as compressed GPU storage or timed BC upload.
inline std::vector<std::uint8_t> decodeDxt(const std::vector<std::uint8_t>& blocks, int width, int height, int dxt)
{
    const int blocksX = (width + 3) / 4, blocksY = (height + 3) / 4;
    const int stride = dxt == 1 ? 8 : 16;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
    for (int by = 0; by < blocksY; ++by)
        for (int bx = 0; bx < blocksX; ++bx)
        {
            const auto* block = &blocks[(by * blocksX + bx) * stride];
            const auto* rgb = block + (dxt == 1 ? 0 : 8);
            const auto c0 = unpackRgb565(static_cast<std::uint16_t>(rgb[0] | (rgb[1] << 8)));
            const auto c1 = unpackRgb565(static_cast<std::uint16_t>(rgb[2] | (rgb[3] << 8)));
            const std::array<std::array<int, 3>, 4> palette{c0,c1,
                std::array<int,3>{(2*c0[0]+c1[0])/3,(2*c0[1]+c1[1])/3,(2*c0[2]+c1[2])/3},
                std::array<int,3>{(c0[0]+2*c1[0])/3,(c0[1]+2*c1[1])/3,(c0[2]+2*c1[2])/3}};
            std::uint32_t indices = 0;
            for (int b = 0; b < 4; ++b) indices |= static_cast<std::uint32_t>(rgb[4+b]) << (8*b);
            std::array<int,8> alphaPalette{};
            std::uint64_t alphaIndices = 0;
            if (dxt == 5)
            {
                alphaPalette[0]=block[0]; alphaPalette[1]=block[1];
                if (block[0] > block[1])
                    for(int i=1;i<=6;++i) alphaPalette[i+1]=((7-i)*block[0]+i*block[1])/7;
                else
                {
                    for(int i=1;i<=4;++i) alphaPalette[i+1]=((5-i)*block[0]+i*block[1])/5;
                    alphaPalette[6]=0; alphaPalette[7]=255;
                }
                for(int b=0;b<6;++b) alphaIndices |= static_cast<std::uint64_t>(block[2+b]) << (8*b);
            }
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 4; ++x)
                {
                    const int px=bx*4+x, py=by*4+y, texel=y*4+x;
                    if(px>=width || py>=height) continue;
                    auto* pixel=&rgba[(py*width+px)*4];
                    const auto& color=palette[(indices >> (2*texel)) & 3];
                    for(int c=0;c<3;++c) pixel[c]=static_cast<std::uint8_t>(color[c]);
                    pixel[3]=dxt==1 ? 255 : (dxt==3 ? static_cast<std::uint8_t>(((block[texel/2] >> ((texel%2)*4)) & 15)*17)
                                                                   : static_cast<std::uint8_t>(alphaPalette[(alphaIndices >> (3*texel)) & 7]));
                }
        }
    return rgba;
}

}
