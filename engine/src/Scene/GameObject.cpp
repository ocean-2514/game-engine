#include "Scene/GameObject.h"

#include <utility>
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

namespace eng {

void GameObject::UpdateComponents(float deltaTime,
    bool fixedDeltaTime) {
    if (!m_isAlive) {
        return;
    }

    m_isTraversingComponents = true;
    {
        struct TraversalGuard {
            bool& traversing;
            ~TraversalGuard() { traversing = false; }
        } guard{m_isTraversingComponents};

        for (auto it = m_components.begin(); it != m_components.end();) {
            if ((*it)->IsAlive()) {
                if (fixedDeltaTime) {
                    (*it)->OnFixedUpdate(deltaTime);
                } else {
                    (*it)->OnUpdate(deltaTime);
                }
            }

            if ((*it)->IsAlive()) {
                ++it;
            } else {
                it = m_components.erase(it);
            }

            if (!m_isAlive) {
                break;
            }
        }
    }

    if (!m_isAlive) {
        return;
    }
    FlushPendingCommands();

    for (auto it = m_children.begin(); it != m_children.end();) {
        GameObject& child = **it;
        if (child.IsAlive()) {
            if (fixedDeltaTime) {
                child.FixedUpdateTree(deltaTime);
            } else {
                child.UpdateTree(deltaTime);
            }
        }
        if (child.IsAlive()) {
            ++it;
        } else {
            it = m_children.erase(it);
        }
    }
}

void GameObject::UpdateTree(float deltaTime) {
    OnUpdate(deltaTime);
    UpdateComponents(deltaTime, false);
}

void GameObject::FixedUpdateTree(float fixedDeltaTime) {
    OnFixedUpdate(fixedDeltaTime);
    UpdateComponents(fixedDeltaTime, true);
}

void GameObject::RenderTree(RenderQueue& queue) {
    if (!m_isAlive) {
        return;
    }

    m_isTraversingComponents = true;
    {
        struct TraversalGuard {
            bool& traversing;
            ~TraversalGuard() { traversing = false; }
        } guard{m_isTraversingComponents};

        for (auto it = m_components.begin(); it != m_components.end();) {
            if ((*it)->IsAlive()) {
                (*it)->OnRender(queue);
            }

            if ((*it)->IsAlive()) {
                ++it;
            } else {
                it = m_components.erase(it);
            }

            if (!m_isAlive) {
                break;
            }
        }
    }

    if (!m_isAlive) {
        return;
    }
    FlushPendingCommands();

    for (auto& child : m_children) {
        if (child->m_isAlive) {
            child->RenderTree(queue);
        }
    }
}

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

GameObject* GameObject::GetChildByName(const std::string& name) {
    if (m_name == name) return this;

    for (const auto& child : m_children) {
        auto* obj = child->GetChildByName(name);
        if (obj) {
            return obj;
        }
    }

    return nullptr;
}

const glm::vec3& GameObject::GetPosition() const {
    return m_position;
}

void GameObject::SetPosition(const glm::vec3& position) {
    m_position = position;
}

const glm::quat& GameObject::GetRotation() const {
    return m_rotation;
}

void GameObject::SetRotation(const glm::quat& rotation) {
    m_rotation = rotation;
}

void GameObject::Rotate(float angle, const glm::vec3& axis) {
    m_rotation *= glm::angleAxis(glm::radians(angle), glm::normalize(axis));
}

const glm::vec3& GameObject::GetScale() const {
    return m_scale;
}

void GameObject::SetScale(const glm::vec3& scale) {
    m_scale = scale;
}

void GameObject::SetLocalTransform(const glm::mat4& transform) {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 skew;
    glm::vec4 perspective;

    const bool success = glm::decompose(
        transform, scale, rotation, position, skew, perspective);

    if (success) {
        SetPosition(position);
        SetRotation(glm::normalize(rotation));
        SetScale(scale);
    } else {
        std::cout << "GameObject::SetLocalTransform: failed to decompose matrix"
            << std::endl;
    }
}

glm::mat4 GameObject::GetLocalTransform() const {
    glm::mat4 model{1.0f};
    model = glm::translate(model, m_position);
    model = model * glm::mat4_cast(m_rotation);
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

glm::vec3 GameObject::GetWorldPosition() const {
    return glm::vec3(GetWorldTransform()[3]);
}

glm::quat GameObject::GetWorldRotation() const {
    if (m_parent == nullptr) {
        return m_rotation;
    } else {
        return m_parent->GetWorldRotation() * m_rotation;
    }
}

glm::vec3 GameObject::GetWorldForward() const {
    return GetWorldRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 GameObject::GetWorldRight() const {
    return GetWorldRotation() * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 GameObject::GetWorldUpward() const {
    return GetWorldRotation() * glm::vec3(0.0f, 1.0f, 0.0f);
}

bool GameObject::IsAlive() const {
    return m_isAlive;
}

void GameObject::MarkForDestroy() {
    m_isAlive = false;
}

bool GameObject::AttachComponent(std::unique_ptr<Component> component) {
    if (!m_isAlive || !component || component->m_owner != nullptr ||
        !component->IsAlive()) {
        return false;
    }

    component->m_owner = this;
    if (m_isTraversingComponents) {
        m_pendingCommands.push_back(
            AttachComponentCommand{std::move(component)});
        return true;
    }

    return AttachComponentImmediate(std::move(component));
}

bool GameObject::AttachComponentImmediate(
    std::unique_ptr<Component> component) {
    if (!component || component->m_owner != this || !component->IsAlive()) {
        return false;
    }
    m_components.push_back(std::move(component));
    return true;
}

void GameObject::FlushPendingCommands() {
    for (auto& command : m_pendingCommands) {
        std::visit([this](auto& value) {
            using CommandType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<CommandType, AttachComponentCommand>) {
                AttachComponentImmediate(std::move(value.component));
            }
        }, command);
    }
    m_pendingCommands.clear();
}

} // namespace eng
