#ifndef O_FONT_ASSET_LOADER
#define O_FONT_ASSET_LOADER

#include "Assets/Font/FontDesc.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace eng {

class FileSystem;
class RenderDevice;
class Font;
class FontFace;
class ShaderProgram;

class FontAssetLoader {
public:
    FontAssetLoader(const FileSystem& fileSystem, RenderDevice& renderDevice);

    // 失败返回 nullptr；失败不写缓存。返回的 Font 可跨 UIContext 共享。
    std::shared_ptr<Font> LoadFont(const FontAssetDesc& desc) const;

    void ClearCache();

private:
    struct FaceCacheKey {
        std::string path;
        int faceIndex = 0;
        bool hinting = true;
        bool lightHinting = false;
        bool operator==(const FaceCacheKey& other) const;
    };
    struct FaceCacheKeyHash {
        std::size_t operator()(const FaceCacheKey& key) const noexcept;
    };
    struct FontCacheKey {
        FaceCacheKey face;
        GlyphAtlasDesc atlas;
        bool operator==(const FontCacheKey& other) const;
    };
    struct FontCacheKeyHash {
        std::size_t operator()(const FontCacheKey& key) const noexcept;
    };

    bool EnsureGlyphShader() const;

    const FileSystem& m_fileSystem;
    RenderDevice& m_renderDevice;
    mutable std::shared_ptr<ShaderProgram> m_glyphShader;
    mutable std::unordered_map<
        FaceCacheKey, std::weak_ptr<FontFace>, FaceCacheKeyHash> m_faces;
    mutable std::unordered_map<
        FontCacheKey, std::weak_ptr<Font>, FontCacheKeyHash> m_fonts;
};

} // namespace eng

#endif // O_FONT_ASSET_LOADER