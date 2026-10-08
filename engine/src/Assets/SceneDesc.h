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
};

} // namespace eng


#endif // O_SCENE_DESC
