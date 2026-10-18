#include "compression_assets.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace compression
{
namespace
{
using Clock = std::chrono::steady_clock;
double milliseconds(Clock::time_point begin, Clock::time_point end)
{
    return std::chrono::duration<double, std::milli>(end - begin).count();
}
std::uint32_t u32(const std::vector<std::uint8_t>& bytes, int offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}
}
std::size_t payloadSize(const Format& format)
{
    if (format.blockSize == 0) return ImageWidth * ImageHeight * 4;
    return static_cast<std::size_t>((ImageWidth + format.blockSize - 1) / format.blockSize) *
           ((ImageHeight + format.blockSize - 1) / format.blockSize) * format.bytesPerBlock;
}
Asset readAsset(const std::string& directory, const Format& format)
{
    const int headerSize = format.preview ? 16 : 128;
    const auto expectedSize = payloadSize(format) + headerSize;
    std::ifstream file(directory + "/" + format.filename, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() != static_cast<std::streamoff>(expectedSize))
        throw std::runtime_error("Missing or invalid prepared texture: " + directory + "/" + format.filename);
    file.seekg(0);
    std::vector<std::uint8_t> bytes(expectedSize);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
        throw std::runtime_error("Cannot read prepared texture payload");
    if (format.preview)
    {
        if (u32(bytes,0) != 0x5ca1ab13 || bytes[4] != format.blockSize || bytes[5] != format.blockSize ||
            bytes[6] != 1 || (u32(bytes,7) & 0xffffff) != ImageWidth ||
            (u32(bytes,10) & 0xffffff) != ImageHeight || bytes[13] != 1 || bytes[14] != 0 || bytes[15] != 0)
            throw std::runtime_error("Invalid ASTC header");
    }
    else
    {
        if (std::memcmp(bytes.data(),"DDS ",4) != 0 || u32(bytes,4) != 124 || u32(bytes,12) != ImageHeight ||
            u32(bytes,16) != ImageWidth || u32(bytes,28) > 1 || u32(bytes,76) != 32)
            throw std::runtime_error("Invalid single-level DDS header");
        if (format.dxt != 0)
        {
            const char fourcc[] = {'D','X','T',static_cast<char>('0'+format.dxt)};
            if (u32(bytes,80) != 4 || std::memcmp(&bytes[84],fourcc,4) != 0)
                throw std::runtime_error("Wrong DDS compressed format");
        }
        else if (u32(bytes,80) != 0x41 || u32(bytes,88) != 32 || u32(bytes,92) != 0xff ||
                 u32(bytes,96) != 0xff00 || u32(bytes,100) != 0xff0000 || u32(bytes,104) != 0xff000000)
            throw std::runtime_error("Expected RGBA8 DDS channel masks");
    }
    return {std::vector<std::uint8_t>(bytes.begin()+headerSize, bytes.end()), expectedSize};
}
GLuint uploadAsset(const Format& format, const Asset& asset)
{
    GLuint texture = 0;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    // All variants use the same base-level-only filtering for a fair comparison.
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    if (format.blockSize == 0)
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,ImageWidth,ImageHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,asset.payload.data());
    else
        glCompressedTexImage2D(GL_TEXTURE_2D,0,format.internalFormat,ImageWidth,ImageHeight,0,
                               static_cast<GLsizei>(asset.payload.size()),asset.payload.data());
    GLint storedFormat = 0, compressed = 0, storedSize = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTERNAL_FORMAT,&storedFormat);
    if (format.blockSize != 0)
    {
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED,&compressed);
        if (compressed) glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED_IMAGE_SIZE,&storedSize);
    }
    const bool accepted = glGetError() == GL_NO_ERROR && storedFormat == static_cast<GLint>(format.internalFormat) &&
        (format.blockSize == 0 || (compressed && storedSize == static_cast<GLint>(asset.payload.size())));
    if (!accepted)
    {
        while (glGetError() != GL_NO_ERROR) {}
        glDeleteTextures(1,&texture); return 0;
    }
    return texture;
}
Measurement benchmark(const std::string& directory, const Format& format, bool supported)
{
    std::array<Measurement,7> samples{};
    // Warm up file caches/driver allocations. Encoding and PNG/JPEG decoding
    // are absent: these are already prepared GPU-ready payloads, as in slides.
    for (int trial = -1; trial < static_cast<int>(samples.size()); ++trial)
    {
        glFinish(); // Exclude earlier rendering from the measurement.
        const auto start = Clock::now();
        const auto asset = readAsset(directory,format);
        const auto readEnd = Clock::now();
        GLuint texture = 0;
        if (supported)
        {
            texture = uploadAsset(format,asset);
            if (!texture) throw std::runtime_error("Prepared texture upload failed during benchmark");
            glFinish(); // Completion, not just CPU command submission.
        }
        const auto end = Clock::now();
        if (texture) glDeleteTextures(1,&texture);
        if (trial >= 0) samples[trial] = {milliseconds(start,readEnd),
                                         supported ? milliseconds(readEnd,end) : 0.0,
                                         supported ? milliseconds(start,end) : 0.0};
    }
    // Keep the read/upload parts from the median TOTAL trial so they add up.
    std::sort(samples.begin(),samples.end(),[supported](const Measurement& a,const Measurement& b) {
        return supported ? a.totalMs < b.totalMs : a.readMs < b.readMs;
    });
    return samples[samples.size()/2];
}
}
