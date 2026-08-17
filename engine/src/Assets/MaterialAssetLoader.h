#ifndef O_MATERIAL_ASSET_LOADER
#define O_MATERIAL_ASSET_LOADER

#include "Assets/MaterialDesc.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace eng {

class FileSystem;
class Material;
class RenderDevice;
class ShaderProgram;
class Texture;

class MaterialAssetLoader {
public:
    MaterialAssetLoader(
        const FileSystem& fileSystem,
        RenderDevice& renderDevice);

    std::shared_ptr<Material> Load(
        const std::string& relativePath) const;
    void ClearCache();

private:
    struct TextureCacheKey {
        std::string path;
        bool srgb = false;
        bool flipVertically = true;
        bool generateMipmaps = true;
        SamplerDesc sampler;

        bool operator==(const TextureCacheKey& other) const;
    };

    struct TextureCacheKeyHash {
        std::size_t operator()(const TextureCacheKey& key) const noexcept;
    };

    bool ParseMaterial(
        const std::string& json,
        const std::string& sourcePath,
        MaterialDesc& result) const;
    std::shared_ptr<Material> CreateMaterial(
        const MaterialDesc& desc) const;
    std::shared_ptr<ShaderProgram> CreateShader(
        const ShaderAssetDesc& desc) const;
    std::shared_ptr<Texture> LoadTexture(
        const TextureAssetDesc& desc) const;
    void PruneExpiredCaches() const;
    void LogError(
        const std::string& sourcePath,
        const std::string& error) const;

    const FileSystem& m_fileSystem;
    RenderDevice& m_renderDevice;
    mutable std::unordered_map<
        ShaderAssetDesc, std::weak_ptr<ShaderProgram>> m_shaderCache;
    mutable std::unordered_map<
        std::string, std::weak_ptr<Material>> m_materialCache;
    mutable std::unordered_map<
        TextureCacheKey,
        std::weak_ptr<Texture>,
        TextureCacheKeyHash> m_textureCache;
};

} // namespace eng

#endif // O_MATERIAL_ASSET_LOADER
