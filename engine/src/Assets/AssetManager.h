#ifndef O_ASSET_MANAGER
#define O_ASSET_MANAGER

#include "Assets/MaterialAssetLoader.h"
#include "Assets/ModelAssetLoader.h"
#include "Assets/TextureAssetLoader.h"
#include "Assets/FontAssetLoader.h"
#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadResult.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Scene/Scene.h"

#include <memory>
#include <string>

namespace eng {

class FileSystem;
class InputManager;
class Material;
class Model;
class RenderDevice;
class Texture;
class AudioManager;
class Font;

class AssetManager {
public:
    AssetManager(const FileSystem& fileSystem, 
        RenderDevice& renderDevice, AudioManager& audioManager);

    std::shared_ptr<Texture> LoadTexture(
        const TextureAssetDesc& desc) const;
    std::shared_ptr<Font> LoadFont(const FontAssetDesc& desc) const;
    std::shared_ptr<Material> LoadMaterial(
        const std::string& relativePath) const;
    std::shared_ptr<Material> InstantiateMaterial(
        const std::string& relativePath) const;
    std::shared_ptr<Model> LoadModel(
        const std::string& relativePath,
        const ModelLoadOptions& options = {});
    SceneLoadResult<SceneDesc> LoadSceneAsset(
        const std::string& relativePath) const;
    SceneLoadResult<std::unique_ptr<Scene>> LoadScene(
        const std::string& relativePath,
        InputManager* input = nullptr);

    SceneTypeRegistry& GetSceneTypeRegistry() noexcept;
    const SceneTypeRegistry& GetSceneTypeRegistry() const noexcept;

    void ClearCaches();

private:
    const FileSystem& m_fileSystem;
    RenderDevice& m_renderDevice;
    AudioManager& m_audioManager;
    SceneTypeRegistry m_sceneTypeRegistry;
    TextureAssetLoader m_textureLoader;
    FontAssetLoader m_fontLoader;
    MaterialAssetLoader m_materialLoader;
    ModelAssetLoader m_modelLoader;
};

} // namespace eng

#endif // O_ASSET_MANAGER
