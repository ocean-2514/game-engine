#include "Assets/Scene/SceneTypeRegistry.h"

namespace eng
{
    
bool SceneTypeRegistry::RegisterComponent(
    std::string stableName, ComponentCodec codec) {
    if (m_frozen || stableName.empty() || !codec.parse ||
        m_components.find(stableName) != m_components.end()) {
        return false;
    }
    return m_components.emplace(
        std::move(stableName), std::move(codec)).second;
}

const ComponentCodec* SceneTypeRegistry::FindComponent(
    const std::string& stableName) const {
    const auto it = m_components.find(stableName);
    if (it == m_components.end()) return nullptr;
    return &it->second;
}

bool SceneTypeRegistry::RegisterGameObject(
    std::string stableName, GameObjectCodec codec) {
    if (m_frozen || stableName.empty() || !codec.parse ||
        m_gameObjects.find(stableName) != m_gameObjects.end()) {
        return false;
    }
    return m_gameObjects.emplace(
        std::move(stableName), std::move(codec)).second;
}

const GameObjectCodec* SceneTypeRegistry::FindGameObject(
    const std::string& stableName) const {
    const auto it = m_gameObjects.find(stableName);
    if (it == m_gameObjects.end()) return nullptr;
    return &it->second;
}



    
} // namespace eng
