#ifndef O_GAME_OBJECT
#define O_GAME_OBJECT

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

#include "Scene/Component.h"

namespace eng {

class Scene;
class RenderQueue;

class GameObject {
public:
    virtual ~GameObject();

    GameObject(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    const std::string& GetName() const;
    void SetName(std::string name);

    GameObject* GetParent();
    const GameObject* GetParent() const;
    std::size_t GetChildCount() const;
    GameObject* GetChild(std::size_t index);
    const GameObject* GetChild(std::size_t index) const;
    GameObject* GetChildByName(const std::string& name);

    const glm::vec3& GetPosition() const;
    glm::vec2 GetPosition2D() const;
    void SetPosition(const glm::vec3& position);
    void SetPosition2D(const glm::vec2& position);
    void SetWorldPosition(const glm::vec3& worldPosition);
    const glm::quat& GetRotation() const;
    float GetRotation2D() const;
    void SetRotation(const glm::quat& rotation);
    void SetRotation2D(float angle);
    void SetWorldRotation(const glm::quat& worldRotation);
    // angle expressed in degrees
    void Rotate(float angle, const glm::vec3& axis);
    const glm::vec3& GetScale() const;
    glm::vec2 GetScale2D() const;
    void SetScale(const glm::vec3& scale);
    void SetScale2D(const glm::vec2& scale);
    void SetLocalTransform(const glm::mat4& tranform);
    void SetLocalTransform(const glm::vec3& pos,
        const glm::quat& rotation, const glm::vec3& scale);
    glm::mat4 GetLocalTransform() const;
    glm::mat4 GetLocalTransform2D() const;
    glm::mat4 GetWorldTransform() const;
    glm::mat4 GetWorldTransform2D() const;
    glm::vec3 GetWorldPosition() const;
    glm::quat GetWorldRotation() const;
    glm::vec3 GetWorldForward() const;
    glm::vec3 GetWorldRight() const;
    glm::vec3 GetWorldUpward() const;

    bool IsAlive() const;
    void MarkForDestroy();

    // create a new component and attach it to the game object
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>);

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = component.get();
        return AttachComponent(std::move(component)) ? result : nullptr;
    }

    template<typename T>
    T* GetComponent() {
        static_assert(std::is_base_of_v<Component, T>);

        for (auto& component : m_components) {
            if (component->IsAlive() &&
                component->GetTypeId() == Component::GetStaticTypeId<T>()) {
                return static_cast<T*>(component.get());
            }
        }

        return nullptr;
    }

    template<typename T>
    const T* GetComponent() const {
        static_assert(std::is_base_of_v<Component, T>);

        for (const auto& component : m_components) {
            if (component->IsAlive() &&
                component->GetTypeId() == Component::GetStaticTypeId<T>()) {
                return static_cast<const T*>(component.get());
            }
        }

        return nullptr;
    }

    template<typename T>
    bool HasComponent() const {
        static_assert(std::is_base_of_v<Component, T>);

        return GetComponent<T>() != nullptr;
    }

    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of_v<Component, T>);

        for (auto& component : m_components) {
            if (component->IsAlive() &&
                component->GetTypeId() == Component::GetStaticTypeId<T>()) {
                component->MarkForDestroy();
                return true;
            }
        }

        return false;
    }

protected:
    GameObject() = default;
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float fixedDeltaTime) {}

private:
    struct AttachComponentCommand {
        std::unique_ptr<Component> component;
    };

    using GameObjectCommand = std::variant<AttachComponentCommand>;

    // called only by AddComponent
    bool AttachComponent(std::unique_ptr<Component> component);
    bool AttachComponentImmediate(std::unique_ptr<Component> component);
    void AttachToScene(Scene& scene);
    void DetachFromScene();
    void DetachComponent(Component& component);
    void FlushPendingCommands();
    void UpdateComponents(float deltaTime,
        bool fixedDeltaTime = false);
    void UpdateTree(float deltaTime);
    void FixedUpdateTree(float fixedDeltaTime);
    void RenderTree(RenderQueue& queue);
    void RenderTree2D(RenderQueue& queue);

    std::string m_name;
    Scene* m_scene = nullptr;
    GameObject* m_parent = nullptr;
    std::vector<std::unique_ptr<GameObject>> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<GameObjectCommand> m_pendingCommands;
    bool m_isTraversingComponents = false;
    bool m_isAlive = true;

    glm::vec3 m_position{.0f, .0f, .0f};
    glm::vec3 m_scale{1.0f, 1.0f, 1.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};

    friend class Scene;
    friend class SceneInstantiator;
};

} // namespace eng

#endif
