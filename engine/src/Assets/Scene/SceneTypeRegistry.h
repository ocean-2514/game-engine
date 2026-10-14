#ifndef O_SCENE_TYPE_REGISTRY
#define O_SCENE_TYPE_REGISTRY

#include "Assets/Scene/SceneLoadResult.h"
#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadContext.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"

#include <rapidjson/document.h>

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace eng
{
    
struct ComponentCodec {
    using ParseFn = std::function<SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>(
            const rapidjson::Value&,
            const SceneParseContext&,
            std::string_view jsonPath
        )>;
    using ResolveFn = std::function<bool(
        const SceneTypePayload&,
        SceneDependencyContext&
    )>;
    using CreateFn = std::function<SceneLoadResult<
        std::unique_ptr<Component>>(
            const SceneTypePayload&,
            const ResolvedSceneAssets&,
            SceneLoadContext&
        )>;

    ParseFn parse;
    ResolveFn resolve;
    CreateFn create;
};  

struct GameObjectCodec {
    using ParseFn = std::function<SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>(
            const rapidjson::Value&,
            const SceneParseContext&,
            std::string_view jsonPath)>;
    using ResolveFn = std::function<bool(
        const SceneTypePayload&,
        SceneDependencyContext&)>;
    using CreateFn = std::function<SceneLoadResult<
        std::unique_ptr<GameObject>>(
            const SceneTypePayload&,
            const ResolvedSceneAssets&,
            SceneLoadContext&)>;

    ParseFn parse;
    ResolveFn resolve;
    CreateFn create;
};

class SceneTypeRegistry {
public:
    bool RegisterComponent(std::string stableName, ComponentCodec codec);
    const ComponentCodec* FindComponent(const std::string& stableName) const;

    bool RegisterGameObject(std::string stableName, GameObjectCodec codec);
    const GameObjectCodec* FindGameObject(const std::string& stableName) const;

    void Freeze() noexcept { m_frozen = true; }
    bool IsFrozen() const noexcept { return m_frozen; }

private:
    std::unordered_map<std::string, ComponentCodec> m_components;
    std::unordered_map<std::string, GameObjectCodec> m_gameObjects;
    bool m_frozen = false;
};
    
} // namespace eng


#endif // O_SCENE_TYPE_REGISTRY
