#include "Scene/Scene.h"

#include <utility>

namespace eng {

void Scene::Update(float deltaTime) {
    if (m_isUpdating) {
        return;
    }
    m_isUpdating = true;

    {
        struct UpdateGuard {
            bool& isUpdating;
            ~UpdateGuard() { isUpdating = false; }
        } guard{m_isUpdating};
    
        for (auto it = m_objects.begin(); it != m_objects.end();) {
            GameObject& object = **it;
            if (object.IsAlive()) {
                object.UpdateTree(deltaTime);
            }
    
            if (object.IsAlive()) {
                ++it;
            } else {
                it = m_objects.erase(it);
            }
        }
    }

    // flush commands after objects marked for destroy are destroyed
    FlushPendingCommands();
}

void Scene::Clear() {
    if (m_isUpdating) {
        m_pendingCommands.push_back(ClearCommand{});
        return;
    }
    ClearImmediate();
}

GameObject* Scene::CreateObject(std::string name, GameObject* parent) {
    std::unique_ptr<GameObject> object(new GameObject());
    GameObject* result = object.get();
    return AttachObject(std::move(object), std::move(name), parent)
        ? result
        : nullptr;
}

bool Scene::AttachObject(
    std::unique_ptr<GameObject> object,
    std::string name,
    GameObject* parent) {
    if (m_isUpdating) {
        if (!object ||
            (parent != nullptr &&
                (!IsKnownObject(parent) || !parent->IsAlive()))) {
            return false;
        }
        m_pendingCommands.push_back(
            AttachObjectCommand{std::move(object), std::move(name), parent}
        );
        return true;
    }

    return AttachObjectImmediate(std::move(object), std::move(name), parent);
}

bool Scene::SetParent(GameObject* object, GameObject* parent) {
    if (m_isUpdating) {
        if (!IsKnownObject(object) || !object->IsAlive() ||
            (parent != nullptr &&
                (!IsKnownObject(parent) || !parent->IsAlive())) ||
            object == parent) {
            return false;
        }
        m_pendingCommands.push_back(
            ReparentCommand{object, parent}
        );
        return true;
    }

    return SetParentImmediate(object, parent);
}

bool Scene::Contains(const GameObject* object) const {
    return object != nullptr && ContainsIn(m_objects, object);
}

bool Scene::ContainsIn(
    const ObjectContainer& objects,
    const GameObject* object) {
    for (const auto& candidate : objects) {
        if (candidate.get() == object || ContainsIn(candidate->m_children, object)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<GameObject> Scene::ExtractFrom(
    ObjectContainer& objects,
    GameObject* object) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->get() == object) {
            auto result = std::move(*it);
            objects.erase(it);
            return result;
        }

        auto result = ExtractFrom((*it)->m_children, object);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

bool Scene::WouldCreateCycle(
    const GameObject* object,
    const GameObject* newParent) {
    for (auto current = newParent; current != nullptr; current = current->m_parent) {
        if (current == object) {
            return true;
        }
    }
    return false;
}

bool Scene::IsStagedObject(const GameObject* object) const {
    if (object == nullptr) {
        return false;
    }

    for (const auto& command : m_pendingCommands) {
        const auto* attach = std::get_if<AttachObjectCommand>(&command);
        if (attach != nullptr && attach->object.get() == object) {
            return true;
        }
    }
    return false;
}

bool Scene::IsKnownObject(const GameObject* object) const {
    return Contains(object) || IsStagedObject(object);
}

void Scene::ClearImmediate() {
    m_objects.clear();
}

bool Scene::SetParentImmediate(GameObject* object, GameObject* parent) {
    if (object == nullptr || !Contains(object) ||
        (parent != nullptr && (!Contains(parent) || !parent->IsAlive())) ||
        WouldCreateCycle(object, parent) || !object->IsAlive()) {
        return false;
    }
    if (object->m_parent == parent) {
        return true;
    }

    auto ownedObject = ExtractFrom(m_objects, object);
    if (!ownedObject) {
        return false;
    }

    object->m_parent = parent;
    if (parent != nullptr) {
        parent->m_children.push_back(std::move(ownedObject));
    } else {
        m_objects.push_back(std::move(ownedObject));
    }
    return true;
}

bool Scene::AttachObjectImmediate(std::unique_ptr<GameObject> object, 
    std::string name, GameObject* parent) {

    if (!object || (parent != nullptr && (!Contains(parent) || !parent->IsAlive())) || 
        !object->IsAlive()) {
        return false;
    }

    object->SetName(std::move(name));
    object->m_parent = parent;
    if (parent != nullptr) {
        parent->m_children.push_back(std::move(object));
    } else {
        m_objects.push_back(std::move(object));
    }
    return true;
}

void Scene::FlushPendingCommands() {
    for (auto& cmd : m_pendingCommands) {
        std::visit([this](auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, AttachObjectCommand>) {
                AttachObjectImmediate(std::move(value.object), std::move(value.name), value.parent);
            } else if constexpr (std::is_same_v<T, ReparentCommand>) {
                SetParentImmediate(value.object, value.parent);
            } else if constexpr (std::is_same_v<T, ClearCommand>) {
                ClearImmediate();
            }
        }, cmd);
    }
    m_pendingCommands.clear();
}

std::size_t Scene::GetRootObjectCount() const {
    return m_objects.size();
}

GameObject* Scene::GetRootObject(std::size_t index) {
    return index < m_objects.size() ? m_objects[index].get() : nullptr;
}

const GameObject* Scene::GetRootObject(std::size_t index) const {
    return index < m_objects.size() ? m_objects[index].get() : nullptr;
}

} // namespace eng
