#include "Assets/Scene/SceneDesc.h"

namespace eng
{
    
bool SceneAssetKey::operator==(const SceneAssetKey& other) const noexcept {
    return other.jsonPath == jsonPath && 
        other.slot == slot;
}

std::size_t SceneAssetKeyHash::operator()(
    const SceneAssetKey& key) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(key.jsonPath);
        std::size_t h2 = std::hash<std::string>{}(key.slot);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
}


} // namespace eng
