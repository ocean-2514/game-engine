#ifndef O_GAME_OBJECT
#define O_GAME_OBJECT

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eng {

class Scene;

class GameObject {
public:
    virtual ~GameObject() = default;

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

    const glm::vec3& GetPosition() const;
    void SetPosition(const glm::vec3& position);
    //rotation angle expressed in radians
    const glm::vec3& GetRotate() const;
    //rotation angle expressed in radians
    void SetRotate(const glm::vec3& rotate);
    const glm::vec3& GetScale() const;
    void SetScale(const glm::vec3& scale);
    glm::mat4 GetLocalTransform() const;
    glm::mat4 GetWorldTransform() const;

    bool IsAlive() const;
    void MarkForDestroy();

protected:
    GameObject() = default;
    virtual void OnUpdate(float deltaTime);

private:
    void UpdateTree(float deltaTime);

    std::string m_name;
    GameObject* m_parent = nullptr;
    std::vector<std::unique_ptr<GameObject>> m_children;
    bool m_isAlive = true;

    glm::vec3 m_position{.0f, .0f, .0f};
    glm::vec3 m_scale{1.0f, 1.0f, 1.0f};
    glm::vec3 m_rotate{.0f, .0f, .0f};

    friend class Scene;
};

} // namespace eng

#endif
