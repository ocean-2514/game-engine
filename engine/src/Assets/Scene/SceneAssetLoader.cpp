#include "Assets/Scene/SceneAssetLoader.h"
#include "Assets/Scene/SceneAssetParser.h"
#include "Assets/Scene/SceneDependencyResolver.h"
#include "Assets/Scene/SceneInstantiator.h"
#include "Assets/Scene/SceneLoadContext.h"

namespace eng
{

SceneAssetLoader::SceneAssetLoader(
    const FileSystem& fileSystem,
    AssetManager& assets,
    RenderDevice& renderDevice,
    const SceneTypeRegistry& registry)
    : m_fileSystem(fileSystem), m_assets(assets),
      m_renderDevice(renderDevice), m_registry(registry) {}

SceneLoadResult<std::unique_ptr<Scene>> SceneAssetLoader::Load(
    const std::string& relativeAssetPath,
    InputManager* input) const {
    SceneAssetParser parser(m_fileSystem, m_registry);
    auto parsed = parser.Load(relativeAssetPath);
    if (!parsed) {
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            *parsed.GetErrors());
    }

    SceneDependencyResolver resolver(
        m_assets, m_renderDevice, m_fileSystem, m_registry);
    auto resolved = resolver.Resolve(parsed.Value());
    if (!resolved) {
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            *resolved.GetErrors());
    }

    SceneLoadContext context{
        m_assets, m_renderDevice, input, parsed.Value().sourcePath, {}};
    SceneInstantiator instantiator(m_registry);
    return instantiator.CreateScene(
        parsed.Value(), resolved.Value(), context);
}

} // namespace eng
