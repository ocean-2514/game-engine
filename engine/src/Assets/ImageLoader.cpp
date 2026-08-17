#include "Assets/ImageLoader.h"

#include "IO/FileSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <algorithm>
#include <iostream>
#include <limits>

namespace eng {

namespace {

TextureFormat GetTextureFormat(int components, bool srgb) {
    switch (components) {
        case 1: return TextureFormat::R8;
        case 2: return TextureFormat::RG8;
        case 3: return srgb ? TextureFormat::SRGB8 : TextureFormat::RGB8;
        case 4: return srgb ? TextureFormat::SRGBA8 : TextureFormat::RGBA8;
        default: return TextureFormat::RGBA8;
    }
}

void FlipRows(
    std::vector<uint8_t>& pixels,
    int width,
    int height,
    int components) {
    const std::size_t rowSize =
        static_cast<std::size_t>(width) * components;
    for (int y = 0; y < height / 2; ++y) {
        auto top = pixels.begin() + static_cast<std::size_t>(y) * rowSize;
        auto bottom = pixels.begin() +
            static_cast<std::size_t>(height - 1 - y) * rowSize;
        std::swap_ranges(top, top + rowSize, bottom);
    }
}

} // namespace

std::optional<ImageData> LoadAssetImage(
    const FileSystem& fileSystem,
    const std::string& relativePath,
    const ImageLoadOptions& options) {
    const auto encoded = fileSystem.LoadAssetFile(relativePath);
    if (encoded.empty()) {
        std::cout << "LoadAssetImage: failed to read asset "
                  << relativePath << '\n';
        return std::nullopt;
    }
    if (encoded.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max())) {
        std::cout << "LoadAssetImage: asset is too large to decode "
                  << relativePath << '\n';
        return std::nullopt;
    }

    int width = 0;
    int height = 0;
    int components = 0;
    stbi_uc* decoded = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(encoded.data()),
        static_cast<int>(encoded.size()),
        &width,
        &height,
        &components,
        0);
    if (decoded == nullptr) {
        std::cout << "LoadAssetImage: failed to decode " << relativePath;
        if (const char* reason = stbi_failure_reason()) {
            std::cout << ": " << reason;
        }
        std::cout << '\n';
        return std::nullopt;
    }

    if (width <= 0 || height <= 0 || components < 1 || components > 4) {
        std::cout << "LoadAssetImage: unsupported image layout for "
                  << relativePath << '\n';
        stbi_image_free(decoded);
        return std::nullopt;
    }

    const std::size_t byteCount = static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height) * components;
    ImageData result;
    result.textureDesc.width = static_cast<uint32_t>(width);
    result.textureDesc.height = static_cast<uint32_t>(height);
    result.textureDesc.format = GetTextureFormat(components, options.srgb);
    result.textureDesc.generateMipmaps = options.generateMipmaps;
    result.pixels.assign(decoded, decoded + byteCount);
    stbi_image_free(decoded);

    if (options.flipVertically) {
        FlipRows(result.pixels, width, height, components);
    }
    return result;
}

} // namespace eng
