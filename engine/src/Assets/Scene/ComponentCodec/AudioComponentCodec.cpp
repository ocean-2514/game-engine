#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "IO/FileSystem.h"
#include "Scene/Components/AudioComponent.h"
#include "Scene/Components/AudioListenerComponent.h"

#include <memory>
#include <string>
#include <utility>

namespace eng {
namespace {

using Codec::GetPayload;
using Codec::MakeParseError;
using Codec::PayloadResult;
using Codec::ReadFiniteFloat;
using Codec::ReadBool;

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseAudio(
    const rapidjson::Value& value, const SceneParseContext& context,
    std::string_view pathView) {
    const std::string path(pathView);
    AudioComponentDesc desc;
    bool valid = true;
    // `audio` keeps compatibility with scenes authored before `asset`.
    const char* assetKey = value.HasMember("asset") ? "asset" : "audio";
    if (!value.HasMember(assetKey)) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "audio asset path is required", path + ".asset");
        valid = false;
    } else {
        valid &= Codec::ParseAssetReference(value[assetKey], desc.assetPath,
            context, path + "." + assetKey);
    }
    valid &= ReadBool(value, "spatial", desc.spatial, context, path);
    valid &= ReadBool(value, "loop", desc.looping, context, path);
    valid &= ReadBool(value, "playOnAttach", desc.playOnAttach, context, path);
    valid &= ReadFiniteFloat(value, "volume", desc.volume, context, path);
    valid &= ReadFiniteFloat(value, "pitch", desc.pitch, context, path);
    valid &= ReadFiniteFloat(value, "minDistance", desc.minDistance, context, path);
    valid &= ReadFiniteFloat(value, "maxDistance", desc.maxDistance, context, path);
    if (value.HasMember("loadMode")) {
        const auto& mode = value["loadMode"];
        if (!mode.IsString() ||
            (std::string(mode.GetString()) != "decode" &&
             std::string(mode.GetString()) != "stream")) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "loadMode must be 'decode' or 'stream'", path + ".loadMode");
            valid = false;
        } else desc.loadMode = std::string(mode.GetString()) == "stream"
            ? AudioLoadMode::Stream : AudioLoadMode::Decode;
    }
    if (desc.volume < 0.0f || desc.pitch <= 0.0f ||
        desc.minDistance < 0.0f || desc.maxDistance < desc.minDistance) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "audio requires volume >= 0, pitch > 0 and 0 <= minDistance <= maxDistance",
            path);
        valid = false;
    }
    if (!valid) return SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>::Failure(
            MakeParseError(context, path, "invalid AudioComponent"));
    return PayloadResult(std::move(desc));
}

bool ResolveAudio(const SceneTypePayload& payload,
    SceneDependencyContext& context) {
    const auto* desc = GetPayload<AudioComponentDesc>(payload);
    if (!desc) {
        context.ReportError(SceneLoadErrorCode::Construction,
            "payload type mismatch for engine.AudioComponent");
        return false;
    }
    if (context.fileSystem.NormalizeAssetPath(desc->assetPath).empty()) {
        context.ReportError(SceneLoadErrorCode::AssetNotFound,
            "audio asset was not found or is outside the assets root",
            desc->assetPath);
        return false;
    }
    return true;
}

SceneLoadResult<std::unique_ptr<Component>> CreateAudio(
    const SceneTypePayload& payload, const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<AudioComponentDesc>(payload);
    if (!desc) return SceneLoadResult<std::unique_ptr<Component>>::Failure(
        context.MakeError(SceneLoadErrorCode::Construction,
            "payload type mismatch for engine.AudioComponent"));
    std::unique_ptr<Component> component = std::make_unique<AudioComponent>(*desc);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(std::move(component));
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseListener(
    const rapidjson::Value&, const SceneParseContext&, std::string_view) {
    return PayloadResult(AudioListenerComponentDesc{});
}

SceneLoadResult<std::unique_ptr<Component>> CreateListener(
    const SceneTypePayload& payload, const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    if (!GetPayload<AudioListenerComponentDesc>(payload))
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.AudioListenerComponent"));
    std::unique_ptr<Component> component =
        std::make_unique<AudioListenerComponent>();
    return SceneLoadResult<std::unique_ptr<Component>>::Success(std::move(component));
}

} // namespace

bool RegisterAudioComponentCodecs(SceneTypeRegistry& registry) {
    ComponentCodec audio;
    audio.parse = ParseAudio;
    audio.resolve = ResolveAudio;
    audio.create = CreateAudio;
    ComponentCodec listener;
    listener.parse = ParseListener;
    listener.resolve = Codec::DefaultResolve;
    listener.create = CreateListener;
    const bool audioRegistered = registry.RegisterComponent(
        "engine.AudioComponent", std::move(audio));
    const bool listenerRegistered = registry.RegisterComponent(
        "engine.AudioListenerComponent", std::move(listener));
    return audioRegistered && listenerRegistered;
}

} // namespace eng
