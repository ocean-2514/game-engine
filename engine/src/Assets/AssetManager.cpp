#include "Assets/AssetManager.h"
#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/SceneAssetLoader.h"
#include "Assets/Scene/SceneAssetParser.h"

namespace eng {

AssetManager::AssetManager(
    const FileSystem& fileSystem,
    RenderDevice& renderDevice,
    AudioManager& audioManager)
    : m_fileSystem(fileSystem),
      m_renderDevice(renderDevice),
      m_audioManager(audioManager),
      m_textureLoader(fileSystem, renderDevice),
      m_fontLoader(fileSystem, renderDevice),
      m_materialLoader(fileSystem, renderDevice, m_textureLoader),
      m_modelLoader(
          fileSystem,
          renderDevice,
          m_textureLoader,
          m_materialLoader) {
    RegisterBuiltinSceneTypes(m_sceneTypeRegistry);
}

std::shared_ptr<Texture> AssetManager::LoadTexture(
    const TextureAssetDesc& desc) const {
    return m_textureLoader.LoadTexture(desc);
}

std::shared_ptr<Material> AssetManager::LoadMaterial(
    const std::string& relativePath) const {
    return m_materialLoader.Load(relativePath);
}

std::shared_ptr<Font> AssetManager::LoadFont(const FontAssetDesc& desc) const {
    return m_fontLoader.LoadFont(desc);
}

std::shared_ptr<Material> AssetManager::InstantiateMaterial(
    const std::string& relativePath) const {
    return m_materialLoader.Instantiate(relativePath);
}

std::shared_ptr<Model> AssetManager::LoadModel(
    const std::string& relativePath,
    const ModelLoadOptions& options) {
    return m_modelLoader.Load(relativePath, options);
}

SceneLoadResult<SceneDesc> AssetManager::LoadSceneAsset(
    const std::string& relativePath) const {
    SceneAssetParser parser(m_fileSystem, m_sceneTypeRegistry);
    return parser.Load(relativePath);
}

SceneLoadResult<std::unique_ptr<Scene>> AssetManager::LoadScene(
    const std::string& relativePath,
    InputManager* input) {
    m_sceneTypeRegistry.Freeze();
    SceneAssetLoader loader(
        m_fileSystem, *this, m_renderDevice,
        m_audioManager,  m_sceneTypeRegistry);
    return loader.Load(relativePath, input);
}

SceneTypeRegistry& AssetManager::GetSceneTypeRegistry() noexcept {
    return m_sceneTypeRegistry;
}

const SceneTypeRegistry& AssetManager::GetSceneTypeRegistry() const noexcept {
    return m_sceneTypeRegistry;
}

void AssetManager::ClearCaches() {
    m_modelLoader.ClearCache();
    m_materialLoader.ClearCache();
    m_textureLoader.ClearCache();
    m_fontLoader.ClearCache();
}

} // namespace eng
