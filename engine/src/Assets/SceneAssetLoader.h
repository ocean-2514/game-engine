#ifndef O_SCENE_ASSET_LOADER
#define O_SCENE_ASSET_LOADER

#include "Assets/SceneLoadResult.h"
#include "SceneDesc.h"

#include <string>

namespace eng
{
    
class FileSystem;
class SceneTypeRegistry;

class SceneAssetLoader
{
public:
    SceneAssetLoader(const FileSystem& filesystem, 
        const SceneTypeRegistry& registry);
    
    SceneLoadResult<SceneDesc> Load(
        const std::string& relativeAssetPath) const;

private:
    const FileSystem& m_filesystem;
    const SceneTypeRegistry& m_registry;
};



} // namespace eng


#endif // O_SCENE_ASSET_LOADER