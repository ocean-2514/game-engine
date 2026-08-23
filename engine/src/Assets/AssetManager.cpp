#include "Assets/AssetManager.h"

namespace eng {

AssetManager::AssetManager(
    const FileSystem& fileSystem,
    RenderDevice& renderDevice)
    : m_textureLoader(fileSystem, renderDevice),
      m_materialLoader(fileSystem, renderDevice, m_textureLoader),
      m_modelLoader(
          fileSystem,
          renderDevice,
          m_textureLoader,
          m_materialLoader) {}

std::shared_ptr<Texture> AssetManager::LoadTexture(
    const TextureAssetDesc& desc) const {
    return m_textureLoader.LoadTexture(desc);
}

std::shared_ptr<Material> AssetManager::LoadMaterial(
    const std::string& relativePath) const {
    return m_materialLoader.Load(relativePath);
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

void AssetManager::ClearCaches() {
    m_modelLoader.ClearCache();
    m_materialLoader.ClearCache();
    m_textureLoader.ClearCache();
}

} // namespace eng
