#ifndef O_SCENE_COMPONENT_CODEC
#define O_SCENE_COMPONENT_CODEC

namespace eng
{

class SceneTypeRegistry;

bool RegisterBuiltinSceneTypes(SceneTypeRegistry& registry);
bool RegisterCameraComponentCodec(SceneTypeRegistry& registry);
bool RegisterRigidBodyComponentCodec(SceneTypeRegistry& registry);
bool RegisterMeshComponentCodec(SceneTypeRegistry& registry);
bool RegisterLightComponentCodec(SceneTypeRegistry& registry);
bool RegisterAudioComponentCodecs(SceneTypeRegistry& registry);

} // namespace eng


#endif // O_SCENE_COMPONENT_CODEC
