// Offline helper: raw RGBA bytes -> DDS RGBA8 / DXT1 / DXT3 / DXT5.
// Rows stay top-to-bottom; the display shader applies the DDS UV convention.
#include "../src/compression_encoders.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

void writeDds(const std::string& path, const std::vector<std::uint8_t>& data, int width, int height, const char* fourcc)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot create DDS: " + path);
    auto word = [&](std::uint32_t value) {
        for (int i = 0; i < 4; ++i) file.put(static_cast<char>(value >> (8 * i)));
    };
    file.write("DDS ", 4);
    word(124); word(fourcc ? 0x81007 : 0x100f); // CAPS | HEIGHT | WIDTH | PIXELFORMAT | LINEARSIZE/PITCH
    word(height); word(width); word(fourcc ? static_cast<std::uint32_t>(data.size()) : width * 4);
    word(0); word(0); // depth, mip count: single base level only
    for (int i = 0; i < 11; ++i) word(0);
    word(32); word(fourcc ? 4 : 0x41); // DDPF_FOURCC or RGB | ALPHAPIXELS
    if (fourcc) file.write(fourcc, 4); else word(0);
    word(fourcc ? 0 : 32);
    word(fourcc ? 0 : 0x000000ff); word(fourcc ? 0 : 0x0000ff00);
    word(fourcc ? 0 : 0x00ff0000); word(fourcc ? 0 : 0xff000000);
    word(0x1000); // DDSCAPS_TEXTURE
    for (int i = 0; i < 4; ++i) word(0);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) throw std::runtime_error("Cannot write DDS payload");
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 5) throw std::runtime_error("Usage: prepare_dxt input.rgba width height output-directory");
        const int width = std::stoi(argv[2]), height = std::stoi(argv[3]);
        if (width < 1 || height < 1 || width > 8192 || height > 8192) throw std::runtime_error("Invalid image dimensions");
        std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
        std::ifstream file(argv[1], std::ios::binary);
        if (!file.read(reinterpret_cast<char*>(rgba.data()), rgba.size())) throw std::runtime_error("Cannot read source RGBA");
        const std::string directory = std::string(argv[4]) + "/";
        writeDds(directory + "landscape_rgba8.dds", rgba, width, height, nullptr);
        writeDds(directory + "landscape_dxt1.dds", compression::encodeDxt1(rgba,width,height), width, height, "DXT1");
        writeDds(directory + "landscape_dxt3.dds", compression::encodeDxt3(rgba,width,height), width, height, "DXT3");
        writeDds(directory + "landscape_dxt5.dds", compression::encodeDxt5(rgba,width,height), width, height, "DXT5");
        std::cout << "Prepared RGBA8, DXT1/BC1, DXT3/BC2, DXT5/BC3 DDS assets\n";
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
