#ifndef O_SCENE_ASSET_LOADER
#define O_SCENE_ASSET_LOADER

#include "Assets/Scene/SceneLoadResult.h"
#include "Scene/Scene.h"

#include <memory>
#include <string>

namespace eng
{

class AssetManager;
class FileSystem;
class InputManager;
class RenderDevice;
class SceneTypeRegistry;
class AudioManager;

class SceneAssetLoader {
public:
    SceneAssetLoader(
        const FileSystem& fileSystem,
        AssetManager& assets,
        RenderDevice& renderDevice,
        AudioManager& audioManager,
        const SceneTypeRegistry& registry);

    SceneLoadResult<std::unique_ptr<Scene>> Load(
        const std::string& relativeAssetPath,
        InputManager* input = nullptr) const;

private:
    const FileSystem& m_fileSystem;
    AssetManager& m_assets;
    RenderDevice& m_renderDevice;
    AudioManager& m_audioManager;
    const SceneTypeRegistry& m_registry;
};

} // namespace eng

#endif // O_SCENE_ASSET_LOADER
