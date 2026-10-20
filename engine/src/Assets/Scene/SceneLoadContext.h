#ifndef O_SCENE_LOAD_CONTEXT
#define O_SCENE_LOAD_CONTEXT

#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadResult.h"

#include <string_view>

namespace eng
{

class AssetManager;
class FileSystem;
class InputManager;
class RenderDevice;
class SceneTypeRegistry;
class AudioManager;

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

    std::string sourcePath;
    std::string jsonPath;
    ResolvedSceneAssets& output;
    std::vector<SceneLoadError>& errors;

    void ReportError(
        SceneLoadErrorCode code,
        std::string message,
        std::string referencedPath = {}) const;
};
    
struct SceneLoadContext {
    AssetManager& assets;
    RenderDevice& renderDevice;
    AudioManager& audioManager;
    InputManager* input = nullptr;
    std::string sourcePath;
    std::string jsonPath;

    SceneLoadError MakeError(
        SceneLoadErrorCode code,
        std::string message) const;
};


} // namespace eng


#endif // O_SCENE_LOAD_CONTEXT
