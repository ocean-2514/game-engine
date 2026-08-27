#ifndef O_SCENE
#define O_SCENE

#include "Scene/GameObject.h"
#include "Scene/Components/CameraComponent.h"
#include "Common.h"

#include <memory>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>

namespace eng {

class Model;

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

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
    std::size_t GetRootObjectCount() const;
    GameObject* GetRootObject(std::size_t index);
    const GameObject* GetRootObject(std::size_t index) const;

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
    // called only in Update()
    void FlushPendingCommands();

    GameObject* ProcessModelNode(
        const Model& model,
        uint32_t nodeIndex,
        GameObject* parent);
    void CollectLightingData(LightingData& data) const;
    void CollectLightingDataRecursive(const GameObject* object, 
        LightingData& data) const;

    ObjectContainer m_objects;
    CameraComponent* m_mainCamera = nullptr;
    bool m_isUpdating = false;
    std::vector<SceneCommand> m_pendingCommands;
};

} // namespace eng

#endif
