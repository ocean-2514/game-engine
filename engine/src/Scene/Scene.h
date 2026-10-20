#ifndef O_SCENE
#define O_SCENE

#include "Scene/GameObject.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Common.h"
#include "Physics/PhysicsWorld.h"

#include <memory>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>

namespace eng {

class Model;
class RigidBodyComponent;
class AudioComponent;
class AudioListenerComponent;
struct SkeletonPose;
class AudioManager;

class Scene {
public:
    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&&) = delete;

    void Update(float deltaTime);
    void Render(RenderQueue& queue, float aspect);
    void Render(RenderQueue& queue, CameraComponent* camera, float aspect);
    // During Update, Clear is staged as an ordinary ordered command.
    void Clear();

    // During Update, a non-null result is a staged object. It is not reported
    // by Contains and is not attached to its parent until command flush.
    GameObject* CreateObject(std::string name, GameObject* parent = nullptr);

    template<typename T, typename... Args,
        std::enable_if_t<std::is_base_of_v<GameObject, T>, int> = 0>
    T* CreateObject(std::string name, GameObject* parent, Args&&... args) {
        auto object = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = object.get();
        return AttachObject(std::move(object), std::move(name), parent)
            ? result
            : nullptr;
    }

    template<typename T,
        std::enable_if_t<std::is_base_of_v<GameObject, T>, int> = 0>
    T* CreateObject(std::string name) {
        return CreateObject<T>(std::move(name), nullptr);
    }

    // Creates an instance root under parent, then reproduces the ModelNode
    // hierarchy below it. Mesh and Material resources remain shared.
    GameObject* InstantiateModel(
        const std::shared_ptr<Model>& model,
        GameObject* parent = nullptr);

    // During Update, true means that the request was accepted for staging;
    // earlier queued commands may still make it invalid before command flush.
    bool SetParent(GameObject* object, GameObject* parent);
    // Contains only reports objects already attached to the scene tree.
    // Objects returned by CreateObject during Update are staged until flush.
    bool Contains(const GameObject* object) const;
    CameraComponent* GetMainCamera();
    const CameraComponent* GetMainCamera() const;
    bool SetMainCamera(CameraComponent* camera);
    bool SetPhysicsWorld(const PhysicsWorldDesc& desc);
    void SetAudioManager(AudioManager* manager);
    AudioManager* GetAudioManager();
    const std::string GetName() const;
    void SetName(std::string name);
    PhysicsWorld* GetPhysicsWorld();
    const PhysicsWorld* GetPhysicsWorld() const;
    const std::vector<PhysicsEvent>& GetCollisionEvents() const;
    std::size_t GetRootObjectCount() const;
    GameObject* GetRootObject(std::size_t index);
    const GameObject* GetRootObject(std::size_t index) const;

    void RegisterRigidBody(RigidBodyComponent* component, PhysicsBodyHandle handle);
    void UnregisterRigidBody(RigidBodyComponent* component);
    void RegisterLight(LightType type, LightComponent* component);
    void UnregisterLight(LightComponent* component);
    void RegisterAudio(AudioComponent* component);
    void UnregisterAudio(AudioComponent* component);
    bool SetAudioListener(AudioListenerComponent* component);
    AudioListenerComponent* GetAudioListener() const;

private:
    using ObjectContainer = std::vector<std::unique_ptr<GameObject>>;

    struct AttachObjectCommand {
        std::unique_ptr<GameObject> object;
        std::string name;
        GameObject* parent = nullptr;
    };

    struct ReparentCommand {
        GameObject* object;
        GameObject* parent;
    };

    struct ClearCommand{};

    using SceneCommand = std::variant<
        AttachObjectCommand, 
        ReparentCommand, 
        ClearCommand
    >;

    struct RegisteredBody {
        RigidBodyComponent* component = nullptr;
        PhysicsBodyHandle handle;
    };

    struct RegisteredLight {
        LightType type = LightType::Point;
        LightComponent* component = nullptr;
    };

    using RegisteredAudio = AudioComponent*;

    // only called during Update
    void UpdateObjects(float deltaTime, bool fixedDeltaTime = false);
    void PushKinematicTransforms();
    void PullDynamicTransforms();
    void RenderInterpolate();
    void RestoreSimulationTransforms();

    // used only for CreateObject()
    bool AttachObject(
        std::unique_ptr<GameObject> object,
        std::string name,
        GameObject* parent);
    static bool ContainsIn(
        const ObjectContainer& objects,
        const GameObject* object);
    static std::unique_ptr<GameObject> ExtractFrom(
        ObjectContainer& objects,
        GameObject* object);
    static bool WouldCreateCycle(
        const GameObject* object,
        const GameObject* newParent);
    bool IsStagedObject(const GameObject* object) const;
    bool IsKnownObject(const GameObject* object) const;
    bool IsValidComponent(const Component* component) const;
        
    void ClearImmediate();
    bool SetParentImmediate(GameObject* object, GameObject* parent);
    bool AttachObjectImmediate(std::unique_ptr<GameObject> object, 
        std::string name, GameObject* parent);
    bool CommitObjects(ObjectContainer roots);
    // called only in Update()
    void FlushPendingCommands();

    static std::unique_ptr<GameObject> BuildDetachedModel(
        const std::shared_ptr<Model>& model);
    static bool BuildDetachedModelNode(
        const Model& model,
        uint32_t nodeIndex,
        GameObject& parent,
        const std::shared_ptr<SkeletonPose>& pose,
        GameObject* modelRoot);
    void CollectLightingData(LightingData& data) const;

    std::unique_ptr<PhysicsWorld> m_physicsWorld;
    float m_physicsAccumulator = 0.0f;
    std::vector<RegisteredBody> m_registeredBodies;

    std::string m_name;
    bool m_isUpdating = false;
    ObjectContainer m_objects;
    CameraComponent* m_mainCamera = nullptr;
    std::vector<RegisteredLight> m_registeredLights;
    std::vector<SceneCommand> m_pendingCommands;
    AudioManager* m_audioManager = nullptr;
    AudioListenerComponent* m_audioListener = nullptr;
    std::vector<RegisteredAudio> m_registeredAudios;

    friend class SceneInstantiator;
};

} // namespace eng

#endif
