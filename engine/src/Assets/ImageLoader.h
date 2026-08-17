#ifndef O_IMAGE_LOADER
#define O_IMAGE_LOADER

#include "Renderer/TextureDesc.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace eng {

class FileSystem;

struct ImageLoadOptions {
    bool srgb = false;
    bool flipVertically = true;
    bool generateMipmaps = true;
};

struct ImageData {
    TextureDesc textureDesc;
    std::vector<uint8_t> pixels;
};

std::optional<ImageData> LoadAssetImage(
    const FileSystem& fileSystem,
    const std::string& relativePath,
    const ImageLoadOptions& options = {});

} // namespace eng

#endif // O_IMAGE_LOADER
