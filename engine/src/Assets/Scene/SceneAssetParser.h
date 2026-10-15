#ifndef O_SCENE_ASSET_PARSER
#define O_SCENE_ASSET_PARSER

#include "Assets/Scene/SceneLoadResult.h"
#include "Assets/Scene/SceneDesc.h"

#include <string>

namespace eng
{
    
class FileSystem;
class SceneTypeRegistry;

class SceneAssetParser
{
public:
    SceneAssetParser(const FileSystem& filesystem, 
        const SceneTypeRegistry& registry);
    
    SceneLoadResult<SceneDesc> Load(
        const std::string& relativeAssetPath) const;

private:
    const FileSystem& m_filesystem;
    const SceneTypeRegistry& m_registry;
};



} // namespace eng


#endif // O_SCENE_ASSET_PARSER