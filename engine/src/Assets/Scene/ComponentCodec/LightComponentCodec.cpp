#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Assets/AssetManager.h"
#include "IO/FileSystem.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <utility>

namespace eng
{

namespace 
{
    bool HasNegativeValues(const glm::vec3& v) {
        return v.x < 0.f || v.y < 0.f || v.z < 0.f;
    }
} // namespace 
    

using Codec::GetPayload;
using Codec::MakeParseError;
using Codec::PayloadResult;
using Codec::ReadFiniteFloat;
using Codec::ReadVec3;
using Codec::DefaultResolve;

bool ParseLightComponent(const rapidjson::Value& value, 
    glm::vec3& color, float& intensity, 
    const SceneParseContext& context, const std::string& path) {
    bool valid = true;
    if (value.HasMember("color")) {
        valid &= ReadVec3(value, color, context, path + ".color");
    }
    valid &= ReadFiniteFloat(value, "intensity", intensity, context, path);
    if (valid && (intensity < 0.0f || HasNegativeValues(color))) {
        context.ReportError(SceneLoadErrorCode::Schema, 
            "light requires intensity >= 0 "
            "and each component of color >= 0", path);
        valid = false;
    }
    return valid;
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParsePointLight(
    const rapidjson::Value& value, 
    const SceneParseContext& context, 
    std::string_view pathView) {
    const std::string path(pathView);
    PointLightComponentDesc desc;
    bool valid = ParseLightComponent(value, 
        desc.color, desc.intensity, context, path);
    valid &= ReadFiniteFloat(value, "range", desc.range, context, path);
    if (valid && desc.range < 0.001f) {
        context.ReportError(SceneLoadErrorCode::Schema, 
            "point light range must be at least 0.001", path + ".range");
        valid = false;
    }
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid PointLightComponent"));
    }
    return PayloadResult(std::move(desc));
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> 
    ParseDirectionalLight(
    const rapidjson::Value& value, 
    const SceneParseContext& context, 
    std::string_view pathView) {
    const std::string path(pathView);
    DirectionalLightComponentDesc desc;
    bool valid = ParseLightComponent(value, 
        desc.color, desc.intensity, context, path);
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid DirectionalLightComponent"));
    }
    return PayloadResult(std::move(desc));
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseSpotLight(
    const rapidjson::Value& value, 
    const SceneParseContext& context, 
    std::string_view pathView) {
    const std::string path(pathView);
    SpotLightComponentDesc desc;
    bool valid = ParseLightComponent(value, 
        desc.color, desc.intensity, context, path);
    valid &= ReadFiniteFloat(value, "range", desc.range, context, path)
        & ReadFiniteFloat(value, "innerConeAngle", desc.innerConeAngle, context, path)
        & ReadFiniteFloat(value, "outerConeAngle", desc.outerConeAngle, context, path);
    if (valid && (desc.range < 0.001f || desc.innerConeAngle < 0.0f ||
        desc.outerConeAngle < desc.innerConeAngle ||
        desc.outerConeAngle > 89.9f)) {
        context.ReportError(SceneLoadErrorCode::Schema, 
            "spot light requires range >= 0.001 and "
            "0 <= innerConeAngle <= outerConeAngle <= 89.9", path);
        valid = false;
    }
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid SpotLightComponent"));
    }
    return PayloadResult(std::move(desc));
}

SceneLoadResult<std::unique_ptr<Component>> CreatePointLight(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<PointLightComponentDesc>(payload);
    if (!desc) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.PointLightComponent"));
    }
    std::unique_ptr<Component> component = 
        std::make_unique<PointLightComponent>(
        desc->color, desc->intensity, desc->range);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

SceneLoadResult<std::unique_ptr<Component>> CreateDirectionalLight(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<DirectionalLightComponentDesc>(payload);
    if (!desc) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.DirectionalLightComponent"));
    }
    std::unique_ptr<Component> component = 
        std::make_unique<DirectionalLightComponent>(
        desc->color, desc->intensity);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

SceneLoadResult<std::unique_ptr<Component>> CreateSpotLight(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<SpotLightComponentDesc>(payload);
    if (!desc) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.SpotLightComponent"));
    }
    std::unique_ptr<Component> component = 
        std::make_unique<SpotLightComponent>(
        desc->color, desc->intensity, desc->range, 
        desc->innerConeAngle, desc->outerConeAngle);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

bool RegisterLightComponentCodec(SceneTypeRegistry& registry) {
    bool success = true;
    
    ComponentCodec point;
    point.parse = ParsePointLight;
    point.resolve = DefaultResolve;
    point.create = CreatePointLight;
    ComponentCodec directional;
    directional.parse = ParseDirectionalLight;
    directional.resolve = DefaultResolve;
    directional.create = CreateDirectionalLight;
    ComponentCodec spot;
    spot.parse = ParseSpotLight;
    spot.resolve = DefaultResolve;
    spot.create = CreateSpotLight;
    
    success &= registry.RegisterComponent("engine.PointLightComponent", std::move(point))
        & registry.RegisterComponent("engine.DirectionalLightComponent", std::move(directional))
        & registry.RegisterComponent("engine.SpotLightComponent", std::move(spot));
    
    return success;
}

} // namespace eng
