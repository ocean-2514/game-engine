#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"

namespace eng
{
    
namespace Codec
{
    SceneLoadError MakeParseError(const SceneParseContext& context,
        std::string_view path, std::string message) {
        return SceneLoadError{
            context.sourcePath, std::string(path), SceneLoadErrorCode::Schema,
            std::move(message), std::nullopt
        };
    }

    bool ParseAssetReference(const rapidjson::Value& value, std::string& output,
        const SceneParseContext& context, const std::string& path) {
        if (!value.IsString() || std::string(value.GetString()).empty()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "asset reference must be a non-empty string", path);
            return false;
        }
        const std::filesystem::path input(value.GetString());
        const auto normalized = input.lexically_normal();
        if (input.is_absolute() || normalized.empty() ||
            *normalized.begin() == "..") {
            context.ReportError(SceneLoadErrorCode::Schema,
                "asset reference must stay relative to the assets root", path);
            return false;
        }
        output = normalized.generic_string();
        return true;
    }

    bool ReadFiniteFloat(const rapidjson::Value& object, const char* name,
        float& output, const SceneParseContext& context,
        std::string_view path, bool required) {
        if (!object.HasMember(name)) {
            if (required) context.ReportError(SceneLoadErrorCode::Schema,
                std::string("missing number '") + name + "'",
                std::string(path) + "." + name);
            return !required;
        }
        const auto& value = object[name];
        if (!value.IsNumber() || !std::isfinite(value.GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                std::string("'") + name + "' must be a finite number",
                std::string(path) + "." + name);
            return false;
        }
        const float parsed = value.GetFloat();
        if (!std::isfinite(parsed)) {
            context.ReportError(SceneLoadErrorCode::Schema,
                std::string("'") + name + "' is outside the finite float range",
                std::string(path) + "." + name);
            return false;
        }
        output = parsed;
        return true;
    }

    bool ReadInteger(const rapidjson::Value& object, const char* name,
        int& output, int minimum, const SceneParseContext& context,
        const std::string& path) {
        if (!object.HasMember(name)) return true;
        if (!object[name].IsInt() || object[name].GetInt() < minimum) {
            context.ReportError(SceneLoadErrorCode::Schema,
                std::string("'") + name + "' must be an integer >= " +
                    std::to_string(minimum),
                path + "." + name);
            return false;
        }
        output = object[name].GetInt();
        return true;
    }

    bool ReadVec3(const rapidjson::Value& value, glm::vec3& output,
        const SceneParseContext& context, std::string_view path) {
        if (!value.IsArray() || value.Size() != 3) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "expected an array of 3 numbers", std::string(path));
            return false;
        }
        for (rapidjson::SizeType i = 0; i < 3; ++i) {
            if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "vector elements must be finite numbers",
                    std::string(path) + "[" + std::to_string(i) + "]");
                return false;
            }
        }
        const glm::vec3 parsed{
            value[0].GetFloat(), value[1].GetFloat(), value[2].GetFloat()};
        for (glm::length_t i = 0; i < 3; ++i) {
            if (!std::isfinite(parsed[i])) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "vector elements must fit in a finite float",
                    std::string(path) + "[" + std::to_string(i) + "]");
                return false;
            }
        }
        output = parsed;
        return true;
    }

    bool ReadQuaternion(const rapidjson::Value& value, glm::quat& output,
        const SceneParseContext& context, std::string_view path) {
        if (!value.IsArray() || value.Size() != 4) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "expected quaternion [x, y, z, w]", std::string(path));
            return false;
        }
        float parts[4]{};
        for (rapidjson::SizeType i = 0; i < 4; ++i) {
            if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "quaternion elements must be finite numbers",
                    std::string(path) + "[" + std::to_string(i) + "]");
                return false;
            }
            parts[i] = value[i].GetFloat();
        }
        output = glm::quat(parts[3], parts[0], parts[1], parts[2]);
        const float length = glm::length(output);
        if (!std::isfinite(length) || length <= 0.0f) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "quaternion must have non-zero finite length", std::string(path));
            return false;
        }
        output = glm::normalize(output);
        return true;
    }

    bool DefaultResolve(const SceneTypePayload&, SceneDependencyContext&) {
        return true;
    }
    
} // namespace Codec


} // namespace eng
