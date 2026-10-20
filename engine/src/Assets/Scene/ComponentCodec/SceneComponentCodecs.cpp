#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/SceneTypeRegistry.h"

namespace eng
{

bool RegisterBuiltinSceneTypes(SceneTypeRegistry& registry) {
    bool success = true;
    success &= RegisterCameraComponentCodec(registry);
    success &= RegisterRigidBodyComponentCodec(registry);
    success &= RegisterMeshComponentCodec(registry);
    success &= RegisterLightComponentCodec(registry);
    success &= RegisterAudioComponentCodecs(registry);
    return success;
}

} // namespace eng
