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
class TextureAssetLoader;

class MaterialAssetLoader {
public:
    MaterialAssetLoader(
        const FileSystem& fileSystem,
        RenderDevice& renderDevice,
        TextureAssetLoader& textureAssetLoader);

    std::shared_ptr<Material> Load(
        const std::string& relativePath) const;
    std::shared_ptr<Material> Instantiate(
        const std::string& relativePath) const;
    void ClearCache();

private:
    bool ParseMaterial(
        const std::string& json,
        const std::string& sourcePath,
        MaterialDesc& result) const;
    std::shared_ptr<Material> CreateMaterial(
        const MaterialDesc& desc) const;
    std::shared_ptr<ShaderProgram> CreateShader(
        const ShaderAssetDesc& desc) const;
    void PruneExpiredCaches() const;
    void LogError(
        const std::string& sourcePath,
        const std::string& error) const;

    const FileSystem& m_fileSystem;
    RenderDevice& m_renderDevice;
    TextureAssetLoader& m_textureAssetLoader;
    mutable std::unordered_map<
        ShaderAssetDesc, std::weak_ptr<ShaderProgram>> m_shaderCache;
    mutable std::unordered_map<
        std::string, std::weak_ptr<Material>> m_materialCache;
};

} // namespace eng

#endif // O_MATERIAL_ASSET_LOADER
