#ifndef O_SCENE_INSTANTIATOR
#define O_SCENE_INSTANTIATOR

#include "Assets/Scene/SceneTypeRegistry.h"
#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadContext.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace eng
{

class Scene;
    
class SceneInstantiator {
public:
    explicit SceneInstantiator(const SceneTypeRegistry& registry);

    SceneLoadResult<std::unique_ptr<Scene>> CreateScene(
        const SceneDesc& desc,
        const ResolvedSceneAssets& resolved,
        SceneLoadContext& context) const;
    
private:
    std::unique_ptr<GameObject> CreateObject(
        const SceneObjectDesc& desc,
        const ResolvedSceneAssets& resolved,
        SceneLoadContext& context, 
        std::vector<SceneLoadError>& errors,
        std::unordered_map<std::string, GameObject*>& objects) const;
    
    std::unique_ptr<Component> CreateComponent(
        const SceneComponentDesc& desc,
        const ResolvedSceneAssets& resolved,
        SceneLoadContext& context, 
        std::vector<SceneLoadError>& errors) const;

    const SceneTypeRegistry& m_registry;
};

} // namespace eng


#endif // O_SCENE_INSTANTIATOR
