#include "Assets/FontAssetLoader.h"
#include "Assets/Font/Font.h"
#include "Assets/Font/FontFace.h"
#include "IO/FileSystem.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/ShaderProgram.h"

namespace eng {

namespace {

template<typename T>
void HashCombine(std::size_t& seed, const T& value) {
	seed ^= std::hash<T>{}(value) + 0x9e3779b9u +
		(seed << 6) + (seed >> 2);
}

} // namespace

bool FontAssetLoader::FaceCacheKey::operator==(const FaceCacheKey& other) const {
    return path == other.path
        && faceIndex == other.faceIndex
        && hinting == other.hinting
        && lightHinting == other.lightHinting;
};

std::size_t FontAssetLoader::FaceCacheKeyHash::operator()(const FaceCacheKey& key) const noexcept {
    std::size_t h = 0;
    HashCombine(h, key.path);
    HashCombine(h, key.faceIndex);
    HashCombine(h, key.hinting);
    HashCombine(h, key.lightHinting);
    return h;
};

bool FontAssetLoader::FontCacheKey::operator==(const FontCacheKey& other) const {
    return face == other.face && atlas == other.atlas;
}

std::size_t FontAssetLoader::FontCacheKeyHash::operator()(const FontCacheKey& key) const noexcept {
    std::size_t h = 0;
    HashCombine(h, FaceCacheKeyHash{}(key.face));
    HashCombine(h, key.atlas.pageWidth);
    HashCombine(h, key.atlas.pageHeight);
    HashCombine(h, key.atlas.maxPages);
    HashCombine(h, key.atlas.padding);
    return h;
};

FontAssetLoader::FontAssetLoader(
    const FileSystem& fileSystem, 
    RenderDevice& renderDevice)
    : m_fileSystem(fileSystem), m_renderDevice(renderDevice) {}

std::shared_ptr<Font> FontAssetLoader::LoadFont(const FontAssetDesc& desc) const {
    const auto normalizedPath = m_fileSystem.NormalizeAssetPath(desc.path);
    if (normalizedPath.empty() || desc.faceIndex < 0 || !GlyphAtlas(desc.atlas).IsValid())
        return nullptr;
    auto faceKey = FaceCacheKey{
        .path = normalizedPath,
        .faceIndex = desc.faceIndex,
        .hinting = desc.rasterization.hinting,
        .lightHinting = desc.rasterization.lightHinting
    };

    auto faceIt = m_faces.find(faceKey);
    std::shared_ptr<FontFace> face;
    if (faceIt != m_faces.end()) {
        if (auto f = faceIt->second.lock()) {
            face = f;
        } else {
            m_faces.erase(faceIt);
        }
    } 
    if (!face) {
        auto fileData = m_fileSystem.LoadAssetFile(
            normalizedPath);
        if (fileData.empty()) {
            return nullptr;
        }
        face = FontFace::Load(
            std::move(fileData),
            desc.faceIndex, desc.rasterization);
        if (!face || !face->IsValid()) {
            return nullptr;
        }
        m_faces.insert_or_assign(faceKey, face);
    }
    
    auto fontKey = FontCacheKey{
        .face = faceKey,
        .atlas = desc.atlas
    };
    auto fontIt = m_fonts.find(fontKey);
    std::shared_ptr<Font> font;
    if (fontIt != m_fonts.end()) {
        if (auto f = fontIt->second.lock()) {
            font = f;
        } else {
            m_fonts.erase(fontIt);
        }
    } 
    if (!font) {
        if (!EnsureGlyphShader()) {
            return nullptr;
        }
        font = std::make_shared<Font>(
            face, m_renderDevice, m_glyphShader, desc.atlas);
        m_fonts.insert_or_assign(fontKey, font);
    }
    return font;
}

void FontAssetLoader::ClearCache() {
    m_faces.clear();
    m_fonts.clear();
}

bool FontAssetLoader::EnsureGlyphShader() const {
    if (m_glyphShader && m_glyphShader->IsValid()) return true;

    auto fsCode = m_fileSystem.LoadAssetTextFile(
        "shader/default/text_quad_fs.glsl");
    auto vsCode = m_fileSystem.LoadAssetTextFile(
        "shader/default/screen_quad_vs.glsl");
    if (fsCode.empty() || vsCode.empty()) {
        return false;
    }
    m_glyphShader = m_renderDevice.CreateShaderProgram(vsCode, fsCode);
    return m_glyphShader != nullptr && m_glyphShader->IsValid();
}

}
