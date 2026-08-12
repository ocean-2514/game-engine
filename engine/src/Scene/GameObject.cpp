#include "Scene/GameObject.h"

#include <utility>

namespace eng {

void GameObject::UpdateTree(float deltaTime) {
    OnUpdate(deltaTime);
    if (!m_isAlive) {
        return;
    }

    for (auto it = m_children.begin(); it != m_children.end();) {
        GameObject& child = **it;
        if (child.IsAlive()) {
            child.UpdateTree(deltaTime);
        }

        if (child.IsAlive()) {
            ++it;
        } else {
            it = m_children.erase(it);
        }
    }
}

void GameObject::OnUpdate(float) {}

const std::string& GameObject::GetName() const {
    return m_name;
}

void GameObject::SetName(std::string name) {
    m_name = std::move(name);
}

GameObject* GameObject::GetParent() {
    return m_parent;
}

const GameObject* GameObject::GetParent() const {
    return m_parent;
}

std::size_t GameObject::GetChildCount() const {
    return m_children.size();
}

GameObject* GameObject::GetChild(std::size_t index) {
    return index < m_children.size() ? m_children[index].get() : nullptr;
}

const GameObject* GameObject::GetChild(std::size_t index) const {
    return index < m_children.size() ? m_children[index].get() : nullptr;
}

const glm::vec3& GameObject::GetPosition() const {
    return m_position;
}

void GameObject::SetPosition(const glm::vec3& position) {
    m_position = position;
}

const glm::vec3& GameObject::GetRotate() const {
    return m_rotate;
}

void GameObject::SetRotate(const glm::vec3& rotate) {
    m_rotate = rotate;
}

const glm::vec3& GameObject::GetScale() const {
    return m_scale;
}

void GameObject::SetScale(const glm::vec3& scale) {
    m_scale = scale;
}

glm::mat4 GameObject::GetLocalTransform() const {
    glm::mat4 model{1.0f};
    model = glm::translate(model, m_position);
    model = glm::rotate(model, m_rotate.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, m_rotate.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, m_rotate.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, m_scale);

    return model;
}

glm::mat4 GameObject::GetWorldTransform() const {
    if (m_parent != nullptr) {
        return m_parent->GetWorldTransform() * GetLocalTransform();
    } else {
        return GetLocalTransform();
    }
}


bool GameObject::IsAlive() const {
    return m_isAlive;
}

void GameObject::MarkForDestroy() {
    m_isAlive = false;
}

} // namespace eng
