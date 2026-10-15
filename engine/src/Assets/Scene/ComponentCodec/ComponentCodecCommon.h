#ifndef O_COMPONENT_CODEC_COMMON
#define O_COMPONENT_CODEC_COMMON

#include "Assets/Scene/SceneDesc.h"
#include "Assets/Scene/SceneLoadResult.h"
#include "Assets/Scene/SceneLoadContext.h"
#include "Assets/AssetManager.h"
#include "Renderer/RenderDevice.h"

#include <rapidjson/document.h>

#include <cmath>
#include <filesystem>
#include <limits>
#include <string>

namespace eng
{
    
namespace Codec
{
    SceneLoadError MakeParseError(const SceneParseContext& context,
        std::string_view path, std::string message);

    bool ParseAssetReference(const rapidjson::Value& value, std::string& output,
        const SceneParseContext& context, const std::string& path);

    bool ReadFiniteFloat(const rapidjson::Value& object, const char* name,
        float& output, const SceneParseContext& context,
        std::string_view path, bool required = false);

    bool ReadInteger(const rapidjson::Value& object, const char* name,
        int& output, int minimum, const SceneParseContext& context,
        const std::string& path);

    bool ReadVec3(const rapidjson::Value& value, glm::vec3& output,
        const SceneParseContext& context, std::string_view path);

    bool ReadQuaternion(const rapidjson::Value& value, glm::quat& output,
        const SceneParseContext& context, std::string_view path);

    template<typename T>
    SceneLoadResult<std::shared_ptr<const SceneTypePayload>> PayloadResult(T value) {
        std::shared_ptr<const SceneTypePayload> payload =
            std::make_shared<SceneTypePayloadValue<T>>(std::move(value));
        return SceneLoadResult<std::shared_ptr<const SceneTypePayload>>::Success(
            std::move(payload));
    }

    template<typename T>
    const T* GetPayload(const SceneTypePayload& payload) {
        const auto* typed = dynamic_cast<const SceneTypePayloadValue<T>*>(&payload);
        return typed ? &typed->value : nullptr;
    }

    bool DefaultResolve(const SceneTypePayload&, SceneDependencyContext&);

} // namespace Codec

} // namespace eng


#endif // O_COMPONENT_CODEC_COMMON