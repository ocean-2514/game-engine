#include "Scene/Component.h"

#include <atomic>

namespace eng {

Component::TypeId Component::AcquireTypeId() noexcept {
    static std::atomic<TypeId> nextId{0};
    return nextId.fetch_add(1, std::memory_order_relaxed);
}
    

GameObject* Component::GetOwner() {
    return m_owner;
}

const GameObject* Component::GetOwner() const {
    return m_owner;
}

void Component::MarkForDestroy() {
    m_isAlive = false;
}

bool Component::IsAlive() const {
    return m_isAlive;
}


} // namespace eng
