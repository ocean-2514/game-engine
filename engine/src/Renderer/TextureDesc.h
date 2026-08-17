#ifndef O_TEXTURE_DESC
#define O_TEXTURE_DESC

#include <cstdint>

namespace eng {

enum class TextureFormat {
    R8,
    RG8,
    RGB8,
    RGBA8,
    SRGB8,
    SRGBA8
};

enum class TextureAddressMode {
    Repeat,
    ClampToEdge,
    MirroredRepeat
};

enum class TextureFilter {
    Nearest,
    Linear
};

struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::RGBA8;
    bool generateMipmaps = true;
};

struct SamplerDesc {
    TextureAddressMode addressU = TextureAddressMode::Repeat;
    TextureAddressMode addressV = TextureAddressMode::Repeat;
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureFilter mipFilter = TextureFilter::Linear;
};

} // namespace eng

#endif // O_TEXTURE_DESC
