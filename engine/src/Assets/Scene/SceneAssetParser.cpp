#include "Assets/Scene/SceneAssetParser.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "IO/FileSystem.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace eng
{
namespace
{

bool ParseVec3(const rapidjson::Value& value, glm::vec3& output,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsArray() || value.Size() != 3) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "expected an array of 3 numbers", path);
        return false;
    }
    for (rapidjson::SizeType i = 0; i < 3; ++i) {
        if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "vector elements must be finite numbers",
                path + "[" + std::to_string(i) + "]");
            return false;
        }
    }
    output = {value[0].GetFloat(), value[1].GetFloat(), value[2].GetFloat()};
    return true;
}

bool ParseQuaternion(const rapidjson::Value& value, glm::quat& output,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsArray() || value.Size() != 4) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "expected quaternion [x, y, z, w]", path);
        return false;
    }
    float parts[4]{};
    for (rapidjson::SizeType i = 0; i < 4; ++i) {
        if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "quaternion elements must be finite numbers",
                path + "[" + std::to_string(i) + "]");
            return false;
        }
        parts[i] = value[i].GetFloat();
    }
    output = glm::quat(parts[3], parts[0], parts[1], parts[2]);
    const float length = glm::length(output);
    if (!std::isfinite(length) || length <= 0.0f) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "quaternion must have non-zero finite length", path);
        return false;
    }
    output = glm::normalize(output);
    return true;
}

bool ParseTransform(const rapidjson::Value& value, SceneTransformDesc& output,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "transform must be an object", path);
        return false;
    }
    bool success = true;
    if (value.HasMember("position")) success &= ParseVec3(
        value["position"], output.position, context, path + ".position");
    if (value.HasMember("rotation")) success &= ParseQuaternion(
        value["rotation"], output.rotation, context, path + ".rotation");
    if (value.HasMember("scale")) {
        success &= ParseVec3(value["scale"], output.scale,
            context, path + ".scale");
        if (output.scale.x == 0.0f || output.scale.y == 0.0f ||
            output.scale.z == 0.0f) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "scale components must be non-zero", path + ".scale");
            success = false;
        }
    }
    return success;
}

void AppendResultErrors(
    const SceneLoadResult<std::shared_ptr<const SceneTypePayload>>& result,
    SceneParseContext& context) {
    if (const auto* errors = result.GetErrors()) {
        context.errors.insert(context.errors.end(), errors->begin(), errors->end());
    }
}

bool ParseComponent(const rapidjson::Value& value,
    const std::string& path, SceneComponentDesc& output,
    SceneParseContext& context) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "component must be an object", path);
        return false;
    }
    if (!value.HasMember("type") || !value["type"].IsString() ||
        std::string(value["type"].GetString()).empty()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "component must have a non-empty string type", path + ".type");
        return false;
    }
    output.typeName = value["type"].GetString();
    output.jsonPath = path;
    const auto* codec = context.registry.FindComponent(output.typeName);
    if (!codec || !codec->parse) {
        context.ReportError(SceneLoadErrorCode::UnknownType,
            "unknown component type '" + output.typeName + "'", path + ".type");
        return false;
    }
    auto result = codec->parse(value, context, path);
    if (!result) {
        AppendResultErrors(result, context);
        return false;
    }
    output.payload = std::move(result.Value());
    if (!output.payload) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "component codec returned an empty payload", path);
        return false;
    }
    return true;
}

bool ParseComponents(const rapidjson::Value& value,
    const std::string& path, std::vector<SceneComponentDesc>& output,
    SceneParseContext& context) {
    if (!value.IsArray()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "components must be an array", path);
        return false;
    }
    bool success = true;
    std::unordered_set<std::string> typeNames;
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
        SceneComponentDesc desc;
        const std::string itemPath = path + "[" + std::to_string(i) + "]";
        if (!ParseComponent(value[i], itemPath, desc, context)) {
            success = false;
        } else if (!typeNames.insert(desc.typeName).second) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "duplicate component type '" + desc.typeName + "'",
                itemPath + ".type");
            success = false;
        } else output.push_back(std::move(desc));
    }
    return success;
}

bool ParseInstance(const rapidjson::Value& value, SceneInstanceDesc& output,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsObject() || !value.HasMember("kind") ||
        !value["kind"].IsString() ||
        std::string(value["kind"].GetString()) != "model" ||
        !value.HasMember("asset") || !value["asset"].IsString()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "instance must contain kind 'model' and an asset path", path);
        return false;
    }
    const std::filesystem::path input(value["asset"].GetString());
    const auto normalized = input.lexically_normal();
    if (input.empty() || input.is_absolute() || normalized.empty() ||
        *normalized.begin() == "..") {
        context.ReportError(SceneLoadErrorCode::Schema,
            "model asset must stay relative to the assets root", path + ".asset");
        return false;
    }
    output.kind = SceneInstanceDesc::Kind::Model;
    output.assetPath = normalized.generic_string();
    return true;
}

bool ParseObject(const rapidjson::Value& value, const std::string& path,
    SceneObjectDesc& output, SceneParseContext& context,
    std::unordered_set<std::string>& ids) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "game object must be an object", path);
        return false;
    }
    bool success = true;
    output.jsonPath = path;
    if (!value.HasMember("id") || !value["id"].IsString() ||
        std::string(value["id"].GetString()).empty()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "object id must be a non-empty string", path + ".id");
        success = false;
    } else {
        output.id = value["id"].GetString();
        if (!ids.insert(output.id).second) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "duplicate object id '" + output.id + "'", path + ".id");
            success = false;
        }
    }
    if (value.HasMember("name")) {
        if (!value["name"].IsString()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "object name must be a string", path + ".name");
            success = false;
        } else output.name = value["name"].GetString();
    } else output.name = output.id;

    if (value.HasMember("class")) {
        if (!value["class"].IsString() ||
            std::string(value["class"].GetString()).empty()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "class must be a non-empty string", path + ".class");
            success = false;
        } else {
            output.className = value["class"].GetString();
            const auto* codec = context.registry.FindGameObject(*output.className);
            if (!codec || !codec->parse) {
                context.ReportError(SceneLoadErrorCode::UnknownType,
                    "unknown game object class '" + *output.className + "'",
                    path + ".class");
                success = false;
            } else {
                auto result = codec->parse(value, context, path);
                if (!result) {
                    AppendResultErrors(result, context);
                    success = false;
                } else output.classPayload = std::move(result.Value());
            }
        }
    }
    if (value.HasMember("transform")) success &= ParseTransform(
        value["transform"], output.transform, context, path + ".transform");
    if (value.HasMember("components")) success &= ParseComponents(
        value["components"], path + ".components", output.components, context);
    if (value.HasMember("instance")) {
        SceneInstanceDesc instance;
        if (ParseInstance(value["instance"], instance,
            context, path + ".instance")) output.instance = std::move(instance);
        else success = false;
    }
    if (output.instance && (value.HasMember("class") ||
        value.HasMember("components") || value.HasMember("children"))) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "model instance cannot also define class, components, or children",
            path);
        success = false;
    }
    if (value.HasMember("children")) {
        if (!value["children"].IsArray()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "children must be an array", path + ".children");
            success = false;
        } else {
            const auto& children = value["children"];
            for (rapidjson::SizeType i = 0; i < children.Size(); ++i) {
                SceneObjectDesc child;
                if (ParseObject(children[i], path + ".children[" +
                    std::to_string(i) + "]", child, context, ids)) {
                    output.children.push_back(std::move(child));
                } else success = false;
            }
        }
    }
    return success;
}

bool ParseObjects(const rapidjson::Value& value, SceneParseContext& context) {
    if (!value.IsArray()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "objects must be an array", "$.objects");
        return false;
    }
    bool success = true;
    std::unordered_set<std::string> ids;
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
        SceneObjectDesc desc;
        if (ParseObject(value[i], "$.objects[" + std::to_string(i) + "]",
            desc, context, ids)) context.scene.objects.push_back(std::move(desc));
        else success = false;
    }
    return success;
}

bool ParsePhysics(const rapidjson::Value& value, SceneParseContext& context) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "physics must be an object", "$.settings.physics");
        return false;
    }
    auto& output = context.scene.settings.physics.world;
    bool success = true;
    if (value.HasMember("gravity")) success &= ParseVec3(
        value["gravity"], output.gravity, context, "$.settings.physics.gravity");
    auto readFloat = [&](const char* name, float& target) {
        if (!value.HasMember(name)) return;
        if (!value[name].IsNumber() || !std::isfinite(value[name].GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                std::string(name) + " must be a finite number",
                std::string("$.settings.physics.") + name);
            success = false;
        } else target = value[name].GetFloat();
    };
    readFloat("fixedTimeStep", output.fixedTimeStep);
    readFloat("maxAccumulatedTime", output.maxAccumulatedTime);
    if (value.HasMember("maxSubStepsPerFrame")) {
        if (!value["maxSubStepsPerFrame"].IsUint()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "maxSubStepsPerFrame must be an unsigned integer",
                "$.settings.physics.maxSubStepsPerFrame");
            success = false;
        } else output.maxSubStepsPerFrame =
            value["maxSubStepsPerFrame"].GetUint();
    }
    if (!output.IsValid()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "invalid physics settings", "$.settings.physics");
        success = false;
    }
    return success;
}

bool ParseSettings(const rapidjson::Value& value, SceneParseContext& context) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "settings must be an object", "$.settings");
        return false;
    }
    bool success = true;
    if (value.HasMember("mainCamera")) {
        if (!value["mainCamera"].IsString() ||
            std::string(value["mainCamera"].GetString()).empty()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "mainCamera must be a non-empty object id",
                "$.settings.mainCamera");
            success = false;
        } else context.scene.settings.mainCameraId =
            value["mainCamera"].GetString();
    }
    if (value.HasMember("physics")) success &=
        ParsePhysics(value["physics"], context);
    return success;
}

void CollectCameraCounts(const SceneObjectDesc& object,
    std::unordered_map<std::string, std::size_t>& cameras) {
    std::size_t count = 0;
    for (const auto& component : object.components) {
        if (component.typeName == "engine.CameraComponent") ++count;
    }
    cameras.emplace(object.id, count);
    for (const auto& child : object.children) CollectCameraCounts(child, cameras);
}

bool ValidateReferences(SceneParseContext& context) {
    if (!context.scene.settings.mainCameraId) return true;
    std::unordered_map<std::string, std::size_t> cameras;
    for (const auto& object : context.scene.objects) CollectCameraCounts(object, cameras);
    const auto it = cameras.find(*context.scene.settings.mainCameraId);
    if (it == cameras.end() || it->second != 1) {
        context.ReportError(SceneLoadErrorCode::InvalidReference,
            "mainCamera must reference an object with one CameraComponent",
            "$.settings.mainCamera");
        return false;
    }
    return true;
}

bool ParseScene(const rapidjson::Value& value, SceneParseContext& context) {
    if (!value.IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "root must be an object", "$");
        return false;
    }
    context.scene.sourcePath = context.sourcePath;
    bool success = true;
    if (!value.HasMember("version") || !value["version"].IsUint()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "version must be an unsigned integer", "$.version");
        success = false;
    } else if (value["version"].GetUint() != 1) {
        context.ReportError(SceneLoadErrorCode::UnsupportedVersion,
            "only scene version 1 is supported", "$.version");
        success = false;
    } else context.scene.version = value["version"].GetUint();
    if (!value.HasMember("name") || !value["name"].IsString() ||
        std::string(value["name"].GetString()).empty()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "name must be a non-empty string", "$.name");
        success = false;
    } else context.scene.name = value["name"].GetString();
    if (value.HasMember("settings")) success &=
        ParseSettings(value["settings"], context);
    if (!value.HasMember("objects")) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "objects array is required", "$.objects");
        success = false;
    } else success &= ParseObjects(value["objects"], context);
    if (success) success &= ValidateReferences(context);
    return success;
}

} // namespace

SceneAssetParser::SceneAssetParser(const FileSystem& filesystem,
    const SceneTypeRegistry& registry)
    : m_filesystem(filesystem), m_registry(registry) {}

SceneLoadResult<SceneDesc> SceneAssetParser::Load(
    const std::string& relativeAssetPath) const {
    const std::string normalizedPath =
        m_filesystem.NormalizeAssetPath(relativeAssetPath);
    if (normalizedPath.empty()) {
        return SceneLoadResult<SceneDesc>::Failure(SceneLoadError{
            relativeAssetPath, {}, SceneLoadErrorCode::Io,
            "scene asset path is invalid or file does not exist", std::nullopt});
    }
    const std::string text = m_filesystem.LoadAssetTextFile(normalizedPath);
    if (text.empty()) {
        return SceneLoadResult<SceneDesc>::Failure(SceneLoadError{
            normalizedPath, {}, SceneLoadErrorCode::Io,
            "scene file is empty or could not be read", std::nullopt});
    }
    rapidjson::Document document;
    document.Parse(text.data(), text.size());
    if (document.HasParseError()) {
        return SceneLoadResult<SceneDesc>::Failure(SceneLoadError{
            normalizedPath, {}, SceneLoadErrorCode::JsonSyntax,
            rapidjson::GetParseError_En(document.GetParseError()),
            document.GetErrorOffset()});
    }
    std::vector<SceneLoadError> errors;
    SceneDesc desc;
    SceneParseContext context{
        m_filesystem, m_registry, normalizedPath, desc, errors};
    if (!ParseScene(document, context)) {
        return SceneLoadResult<SceneDesc>::Failure(std::move(errors));
    }
    return SceneLoadResult<SceneDesc>::Success(std::move(desc));
}

} // namespace eng
