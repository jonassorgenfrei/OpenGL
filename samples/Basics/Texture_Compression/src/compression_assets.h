#pragma once
#include <glad/glad.h>
#include <cstdint>
#include <string>
#include <vector>

namespace compression
{
constexpr int ImageWidth = 1024;
constexpr int ImageHeight = 1024;
struct Format
{
    const char* name;
    const char* filename;
    const char* description;
    GLenum internalFormat;
    int blockSize;
    int bytesPerBlock;
    int dxt; // 0 for source/ASTC; otherwise 1, 3, or 5
    const char* preview; // Offline ASTC decode for GPUs without ASTC support.
};
struct Asset
{
    std::vector<std::uint8_t> payload;
    std::size_t fileBytes = 0;
};
struct Measurement
{
    double readMs = 0;
    double uploadMs = 0;
    double totalMs = 0;
};
std::size_t payloadSize(const Format& format);
Asset readAsset(const std::string& directory, const Format& format);
GLuint uploadAsset(const Format& format, const Asset& asset);
Measurement benchmark(const std::string& directory, const Format& format, bool supported);
}
