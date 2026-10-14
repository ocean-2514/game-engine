#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Scene/Components/CameraComponent.h"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <utility>

namespace eng
{

namespace
{

using Codec::GetPayload;
using Codec::MakeParseError;
using Codec::PayloadResult;
using Codec::ReadFiniteFloat;

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseCamera(
    const rapidjson::Value& value,
    const SceneParseContext& context,
    std::string_view path) {
    CameraComponentDesc desc;
    bool valid = ReadFiniteFloat(value, "fov", desc.fov, context, path) &&
        ReadFiniteFloat(value, "near", desc.nearPlane, context, path) &&
        ReadFiniteFloat(value, "far", desc.farPlane, context, path);
    if (valid && (desc.fov <= 0.0f || desc.fov >= 180.0f ||
        desc.nearPlane <= 0.0f || desc.farPlane <= desc.nearPlane)) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "camera requires 0 < fov < 180 and 0 < near < far",
            std::string(path));
        valid = false;
    }
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid CameraComponent"));
    }
    return PayloadResult(std::move(desc));
}

bool ResolveCamera(const SceneTypePayload&, SceneDependencyContext&) {
    return true;
}

SceneLoadResult<std::unique_ptr<Component>> CreateCamera(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<CameraComponentDesc>(payload);
    if (!desc) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.CameraComponent"));
    }
    std::unique_ptr<Component> component = std::make_unique<CameraComponent>(
        context.input, desc->fov, desc->nearPlane, desc->farPlane);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

} // namespace

bool RegisterCameraComponentCodec(SceneTypeRegistry& registry) {
    ComponentCodec codec;
    codec.parse = ParseCamera;
    codec.resolve = ResolveCamera;
    codec.create = CreateCamera;
    return registry.RegisterComponent("engine.CameraComponent",
        std::move(codec));
}

} // namespace eng
