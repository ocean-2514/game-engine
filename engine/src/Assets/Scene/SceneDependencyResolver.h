#ifndef O_SCENE_DEPENDENCY_RESOLVER
#define O_SCENE_DEPENDENCY_RESOLVER

#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadResult.h"

namespace eng
{

class AssetManager;
class FileSystem;
class RenderDevice;
class SceneTypeRegistry;
     
class SceneDependencyResolver {
public:
    SceneDependencyResolver(
        AssetManager& assets,
        RenderDevice& renderDevice,
        const FileSystem& fileSystem,
        const SceneTypeRegistry& registry);

    SceneLoadResult<ResolvedSceneAssets> Resolve(
        const SceneDesc& desc) const;
        
private:
    bool ResolveObject(
        const std::string& sceneSrc,
        const SceneObjectDesc& desc, 
        ResolvedSceneAssets& output,
        std::vector<SceneLoadError>& errors) const;

    AssetManager& m_assetManager;
    RenderDevice& m_renderDevice;
    const FileSystem& m_fileSystem;
    const SceneTypeRegistry& m_registry;
};


    
} // namespace eng


#endif // O_SCENE_DEPENDENCY_RESOLVER