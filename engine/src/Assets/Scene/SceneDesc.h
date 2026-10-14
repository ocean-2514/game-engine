#ifndef O_SCENE_DESC
#define O_SCENE_DESC

#include "Physics/PhysicsTypes.h"
#include "Renderer/MeshShape.h"

#include <glm/glm.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>

namespace eng
{
    
struct SceneTransformDesc {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct ScenePhysicsDesc {
    PhysicsWorldDesc world;
};

// component desc
struct CameraComponentDesc {
    float fov = 45.0f;
    float nearPlane = 0.01f;
    float farPlane = 100.0f;
};

struct RigidBodyComponentDesc {
    RigidBodyDesc body;
};

struct MeshComponentDesc {
    MeshShapeDesc shape;
    std::string materialPath;
};

class SceneTypePayload {
public:
    virtual ~SceneTypePayload() = default;
};

template<typename T>
class SceneTypePayloadValue final : public SceneTypePayload {
public:
    explicit SceneTypePayloadValue(T value)
        : value(std::move(value)) {}
    
    T value;
};

struct SceneComponentDesc {
    std::string typeName;
    std::shared_ptr<const SceneTypePayload> payload;
    std::string jsonPath;
};

struct SceneInstanceDesc {
    enum class Kind {
        Model
    };

    Kind kind = Kind::Model;
    std::string assetPath;
};

struct SceneObjectDesc {
    std::string id;
    std::string name;
    std::optional<std::string> className;
    std::shared_ptr<const SceneTypePayload> classPayload;
    std::string jsonPath;
    SceneTransformDesc transform;
    std::optional<SceneInstanceDesc> instance;
    std::vector<SceneComponentDesc> components;
    std::vector<SceneObjectDesc> children;
};

struct SceneSettingsDesc {
    std::optional<std::string> mainCameraId;
    ScenePhysicsDesc physics;
};

struct SceneDesc {
    uint32_t version = 1;
    std::string name;
    SceneSettingsDesc settings;
    std::vector<SceneObjectDesc> objects;
    std::string sourcePath;
};

struct SceneAssetKey {
    std::string jsonPath;
    std::string slot; // "mesh", "material", "model"

    bool operator==(const SceneAssetKey& other) const noexcept;
};

struct SceneAssetKeyHash {
    std::size_t operator()(const SceneAssetKey& key) const noexcept;
};

class ResolvedAsset {
public:
    virtual ~ResolvedAsset() = default;
};

template<typename T>
class ResolvedAssetValue final : public ResolvedAsset {
public:
    explicit ResolvedAssetValue(std::shared_ptr<T> value)
        : value(std::move(value)) {}

    std::shared_ptr<T> value;
};

class ResolvedSceneAssets {
public:
    template<typename T>
    void Store(SceneAssetKey key, std::shared_ptr<T> value) {
        m_assets.insert_or_assign(
            std::move(key),
            std::make_shared<ResolvedAssetValue<T>>(std::move(value)));
    }

    template<typename T>
    std::shared_ptr<T> Find(const SceneAssetKey& key) const {
        const auto it = m_assets.find(key);
        if (it == m_assets.end()) return nullptr;
        const auto* value = dynamic_cast<const ResolvedAssetValue<T>*>(
            it->second.get());
        return value ? value->value : nullptr;
    }

private:
    std::unordered_map<
        SceneAssetKey,
        std::shared_ptr<const ResolvedAsset>,
        SceneAssetKeyHash> m_assets;
};

} // namespace eng


#endif // O_SCENE_DESC
