#ifndef O_SCENE_LOAD_CONTEXT
#define O_SCENE_LOAD_CONTEXT

#include "Assets/SceneDesc.h"
#include "Assets/SceneLoadResult.h"

#include <string_view>

namespace eng
{

class AssetManager;
class FileSystem;
class InputManager;
class RenderDevice;
class SceneTypeRegistry;

struct SceneParseContext {
    const FileSystem& filesystem;
    const SceneTypeRegistry& registry;
    const std::string& sourcePath;
    SceneDesc& scene;
    std::vector<SceneLoadError>& errors;

    void ReportError(
        SceneLoadErrorCode code,
        std::string message, std::string jsonPath = "",
        std::optional<std::size_t> parseOffset = std::nullopt) const;
};

struct SceneDependencyContext {
    const FileSystem& fileSystem;
    AssetManager& assets;
    RenderDevice& renderDevice;

    std::string_view sourcePath;
    std::string jsonPath;
    ResolvedSceneAssets& output;

    SceneLoadError ReportError(
        SceneLoadErrorCode code,
        std::string message) const;
};
    
struct SceneLoadContext {
    AssetManager& assets;
    RenderDevice& renderDevice;
    InputManager* input = nullptr;
};


} // namespace eng


#endif // O_SCENE_LOAD_CONTEXT
