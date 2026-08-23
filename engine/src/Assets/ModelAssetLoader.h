#ifndef O_MODEL_ASSET_LOADER
#define O_MODEL_ASSET_LOADER

#include <memory>
#include <string>
#include <unordered_map>

namespace eng {

class FileSystem;
class MaterialAssetLoader;
class Model;
class RenderDevice;
class TextureAssetLoader;

enum class AlphaTextureFallback {
    Opaque,
    Masked,
    Transparent
};

struct ModelLoadOptions {
    std::string materialTemplatePath = "material/model-default.json";
    bool importMaterials = true;
    AlphaTextureFallback alphaTextureFallback =
        AlphaTextureFallback::Masked;
};

class ModelAssetLoader {
public:
    ModelAssetLoader(
        const FileSystem& fileSystem,
        RenderDevice& renderDevice,
        TextureAssetLoader& textureAssetLoader,
        MaterialAssetLoader& materialAssetLoader);

    std::shared_ptr<Model> Load(
        const std::string& relativePath,
        const ModelLoadOptions& options = {});
    void ClearCache();

private:
    const FileSystem& m_fileSystem;
    RenderDevice& m_renderDevice;
    TextureAssetLoader& m_textureAssetLoader;
    MaterialAssetLoader& m_materialAssetLoader;
    std::unordered_map<std::string, std::weak_ptr<Model>> m_modelCache;
};

} // namespace eng

#endif // O_MODEL_ASSET_LOADER
