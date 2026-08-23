#include "Assets/MaterialAssetLoader.h"
#include "Assets/TextureAssetLoader.h"

#include "IO/FileSystem.h"
#include "Renderer/Material.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Texture.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace eng {

namespace {

template<typename Map>
void EraseExpired(Map& cache) {
    for (auto it = cache.begin(); it != cache.end();) {
        if (it->second.expired()) it = cache.erase(it);
        else ++it;
    }
}

bool IsNumberArray(const rapidjson::Value& value, rapidjson::SizeType size) {
    if (!value.IsArray() || value.Size() != size) {
        return false;
    }
    for (const auto& element : value.GetArray()) {
        if (!element.IsNumber()) {
            return false;
        }
    }
    return true;
}

bool ParseAddressMode(
    const rapidjson::Value& value,
    TextureAddressMode& result) {
    if (!value.IsString()) return false;
    const std::string mode = value.GetString();
    if (mode == "repeat") result = TextureAddressMode::Repeat;
    else if (mode == "clampToEdge") result = TextureAddressMode::ClampToEdge;
    else if (mode == "mirroredRepeat") result = TextureAddressMode::MirroredRepeat;
    else return false;
    return true;
}

bool ParseFilter(const rapidjson::Value& value, TextureFilter& result) {
    if (!value.IsString()) return false;
    const std::string filter = value.GetString();
    if (filter == "nearest") result = TextureFilter::Nearest;
    else if (filter == "linear") result = TextureFilter::Linear;
    else return false;
    return true;
}

bool ParseSurfaceMode(const rapidjson::Value& value, SurfaceMode& result) {
    if (!value.IsString()) return false;
    const std::string mode = value.GetString();
    if (mode == "opaque") result = SurfaceMode::Opaque;
    else if (mode == "masked") result = SurfaceMode::Masked;
    else if (mode == "transparent") result = SurfaceMode::Transparent;
    else return false;
    return true;
}

bool ParseCompareOp(const rapidjson::Value& value, CompareOp& result) {
    if (!value.IsString()) return false;
    const std::string op = value.GetString();
    if (op == "less") result = CompareOp::Less;
    else if (op == "lessEqual") result = CompareOp::LessEqual;
    else if (op == "greater") result = CompareOp::Greater;
    else if (op == "greaterEqual") result = CompareOp::GreaterEqual;
    else if (op == "never") result = CompareOp::Never;
    else if (op == "always") result = CompareOp::Always;
    else if (op == "equal") result = CompareOp::Equal;
    else if (op == "notEqual") result = CompareOp::NotEqual;
    else return false;
    return true;
}

bool ParseBlendFactor(const rapidjson::Value& value, BlendFactor& result) {
    if (!value.IsString()) return false;
    const std::string factor = value.GetString();
    if (factor == "zero") result = BlendFactor::Zero;
    else if (factor == "one") result = BlendFactor::One;
    else if (factor == "sourceAlpha") result = BlendFactor::SourceAlpha;
    else if (factor == "oneMinusSourceAlpha") {
        result = BlendFactor::OneMinusSourceAlpha;
    } else return false;
    return true;
}

bool ParseBlendOp(const rapidjson::Value& value, BlendOp& result) {
    if (!value.IsString()) return false;
    const std::string op = value.GetString();
    if (op == "add") result = BlendOp::Add;
    else if (op == "subtract") result = BlendOp::Subtract;
    else if (op == "reverseSubtract") result = BlendOp::ReverseSubtract;
    else if (op == "min") result = BlendOp::Min;
    else if (op == "max") result = BlendOp::Max;
    else return false;
    return true;
}

bool ParseCullMode(const rapidjson::Value& value, CullMode& result) {
    if (!value.IsString()) return false;
    const std::string mode = value.GetString();
    if (mode == "none") result = CullMode::None;
    else if (mode == "front") result = CullMode::Front;
    else if (mode == "back") result = CullMode::Back;
    else return false;
    return true;
}

bool ParseFrontFace(const rapidjson::Value& value, FrontFace& result) {
    if (!value.IsString()) return false;
    const std::string face = value.GetString();
    if (face == "clockwise") result = FrontFace::Clockwise;
    else if (face == "counterClockwise") {
        result = FrontFace::CounterClockwise;
    } else return false;
    return true;
}

bool ParseRenderDesc(
    const rapidjson::Value& value,
    MaterialRenderDesc& result,
    std::string& error) {
    if (!value.IsObject()) {
        error = "render must be an object";
        return false;
    }
    if (value.HasMember("surface")) {
        if (!ParseSurfaceMode(value["surface"], result.surface)) {
            error = "render.surface must be opaque, masked or transparent";
            return false;
        }
        result.state = MakeRenderState(result.surface);
    }
    if (value.HasMember("order")) {
        if (!value["order"].IsInt()) {
            error = "render.order must be an integer";
            return false;
        }
        result.defaultRenderOrder = value["order"].GetInt();
    }
    if (value.HasMember("alphaCutoff")) {
        if (!value["alphaCutoff"].IsNumber()) {
            error = "render.alphaCutoff must be a number";
            return false;
        }
        result.alphaCutoff = value["alphaCutoff"].GetFloat();
        if (result.alphaCutoff < 0.0f || result.alphaCutoff > 1.0f) {
            error = "render.alphaCutoff must be between 0 and 1";
            return false;
        }
    }
    if (value.HasMember("depth")) {
        const auto& depth = value["depth"];
        if (!depth.IsObject()) {
            error = "render.depth must be an object";
            return false;
        }
        if (depth.HasMember("test")) {
            if (!depth["test"].IsBool()) {
                error = "render.depth.test must be a boolean";
                return false;
            }
            result.state.depth.depthTestEnable = depth["test"].GetBool();
        }
        if (depth.HasMember("write")) {
            if (!depth["write"].IsBool()) {
                error = "render.depth.write must be a boolean";
                return false;
            }
            result.state.depth.depthWriteEnable = depth["write"].GetBool();
        }
        if (depth.HasMember("compare") &&
            !ParseCompareOp(depth["compare"],
                result.state.depth.depthCompareOp)) {
            error = "render.depth.compare is invalid";
            return false;
        }
    }
    if (value.HasMember("blend")) {
        const auto& blend = value["blend"];
        if (!blend.IsObject()) {
            error = "render.blend must be an object";
            return false;
        }
        if (blend.HasMember("enable")) {
            if (!blend["enable"].IsBool()) {
                error = "render.blend.enable must be a boolean";
                return false;
            }
            result.state.blend.blendEnable = blend["enable"].GetBool();
        }
        if (blend.HasMember("source") &&
            !ParseBlendFactor(blend["source"],
                result.state.blend.sourceColor)) {
            error = "render.blend.source is invalid";
            return false;
        }
        if (blend.HasMember("destination") &&
            !ParseBlendFactor(blend["destination"],
                result.state.blend.destinationColor)) {
            error = "render.blend.destination is invalid";
            return false;
        }
        if (blend.HasMember("operation") &&
            !ParseBlendOp(blend["operation"],
                result.state.blend.colorOperation)) {
            error = "render.blend.operation is invalid";
            return false;
        }
    }
    if (value.HasMember("rasterizer")) {
        const auto& rasterizer = value["rasterizer"];
        if (!rasterizer.IsObject()) {
            error = "render.rasterizer must be an object";
            return false;
        }
        if (rasterizer.HasMember("cull") &&
            !ParseCullMode(rasterizer["cull"],
                result.state.rasterizer.cullMode)) {
            error = "render.rasterizer.cull is invalid";
            return false;
        }
        if (rasterizer.HasMember("frontFace") &&
            !ParseFrontFace(rasterizer["frontFace"],
                result.state.rasterizer.frontFace)) {
            error = "render.rasterizer.frontFace is invalid";
            return false;
        }
    }
    return true;
}

bool ParseTextureOptions(
    const rapidjson::Value& parameter,
    TextureAssetDesc& result,
    std::string& error) {
    if (!parameter.HasMember("options")) return true;
    const auto& options = parameter["options"];
    if (!options.IsObject()) {
        error = "texture options must be an object";
        return false;
    }

    if (options.HasMember("colorSpace")) {
        if (!options["colorSpace"].IsString()) {
            error = "texture colorSpace must be a string";
            return false;
        }
        const std::string colorSpace = options["colorSpace"].GetString();
        if (colorSpace == "sRGB") result.loadOptions.srgb = true;
        else if (colorSpace == "linear") result.loadOptions.srgb = false;
        else {
            error = "texture colorSpace must be 'sRGB' or 'linear'";
            return false;
        }
    }
    if (options.HasMember("generateMipmaps")) {
        if (!options["generateMipmaps"].IsBool()) {
            error = "texture generateMipmaps must be a boolean";
            return false;
        }
        result.loadOptions.generateMipmaps =
            options["generateMipmaps"].GetBool();
    }
    if (options.HasMember("flipVertically")) {
        if (!options["flipVertically"].IsBool()) {
            error = "texture flipVertically must be a boolean";
            return false;
        }
        result.loadOptions.flipVertically =
            options["flipVertically"].GetBool();
    }
    if (options.HasMember("addressU") &&
        !ParseAddressMode(options["addressU"], result.samplerDesc.addressU)) {
        error = "texture addressU is invalid";
        return false;
    }
    if (options.HasMember("addressV") &&
        !ParseAddressMode(options["addressV"], result.samplerDesc.addressV)) {
        error = "texture addressV is invalid";
        return false;
    }
    if (options.HasMember("minFilter") &&
        !ParseFilter(options["minFilter"], result.samplerDesc.minFilter)) {
        error = "texture minFilter is invalid";
        return false;
    }
    if (options.HasMember("magFilter") &&
        !ParseFilter(options["magFilter"], result.samplerDesc.magFilter)) {
        error = "texture magFilter is invalid";
        return false;
    }
    if (options.HasMember("mipFilter") &&
        !ParseFilter(options["mipFilter"], result.samplerDesc.mipFilter)) {
        error = "texture mipFilter is invalid";
        return false;
    }
    return true;
}

} // namespace

MaterialAssetLoader::MaterialAssetLoader(
    const FileSystem& fileSystem,
    RenderDevice& renderDevice,
    TextureAssetLoader& textureAssetLoader)
    : m_fileSystem(fileSystem),
      m_renderDevice(renderDevice),
      m_textureAssetLoader(textureAssetLoader) {}

std::shared_ptr<Material> MaterialAssetLoader::Load(
    const std::string& relativePath) const {
    const std::string normalizedPath =
        m_fileSystem.NormalizeAssetPath(relativePath);
    if (normalizedPath.empty()) {
        LogError(relativePath, "material asset path is invalid");
        return nullptr;
    }

    PruneExpiredCaches();
    const auto cached = m_materialCache.find(normalizedPath);
    if (cached != m_materialCache.end()) {
        if (auto material = cached->second.lock()) {
            return material;
        }
        m_materialCache.erase(cached);
    }

    const std::string json =
        m_fileSystem.LoadAssetTextFile(normalizedPath);
    if (json.empty()) {
        LogError(normalizedPath, "material file is empty or unreadable");
        return nullptr;
    }

    MaterialDesc desc;
    if (!ParseMaterial(json, normalizedPath, desc)) {
        return nullptr;
    }

    auto material = CreateMaterial(desc);
    if (material) {
        m_materialCache.insert_or_assign(normalizedPath, material);
    }
    return material;
}

std::shared_ptr<Material> MaterialAssetLoader::Instantiate(
    const std::string& relativePath) const {
    const auto material = Load(relativePath);
    return material ? material->Clone() : nullptr;
}

void MaterialAssetLoader::ClearCache() {
    m_materialCache.clear();
    m_shaderCache.clear();
}

bool MaterialAssetLoader::ParseMaterial(const std::string& json,
    const std::string& sourcePath, MaterialDesc& result) const {
    rapidjson::Document doc;
    doc.Parse(json.c_str(), json.size());
    if (doc.HasParseError()) {
        LogError(sourcePath,
            std::string("JSON parse error at byte ") +
            std::to_string(doc.GetErrorOffset()) + ": " +
            rapidjson::GetParseError_En(doc.GetParseError()));
        return false;
    }
    if (!doc.IsObject()) {
        LogError(sourcePath, "root must be an object");
        return false;
    }
    if (!doc.HasMember("version") || !doc["version"].IsUint() ||
        doc["version"].GetUint() != 1) {
        LogError(sourcePath, "version must be the unsigned integer 1");
        return false;
    }
    result.version = doc["version"].GetUint();

    if (doc.HasMember("name")) {
        if (!doc["name"].IsString()) {
            LogError(sourcePath, "name must be a string");
            return false;
        }
        result.name = doc["name"].GetString();
    }

    if (doc.HasMember("render")) {
        std::string renderError;
        if (!ParseRenderDesc(doc["render"], result.render, renderError)) {
            LogError(sourcePath, renderError);
            return false;
        }
    }

    if (!doc.HasMember("shader") || !doc["shader"].IsObject()) {
        LogError(sourcePath, "shader must be an object");
        return false;
    }
    const auto& shader = doc["shader"];
    if (!shader.HasMember("vertex") || !shader["vertex"].IsString() ||
        !shader.HasMember("fragment") || !shader["fragment"].IsString()) {
        LogError(sourcePath,
            "shader.vertex and shader.fragment must be strings");
        return false;
    }
    result.shader.vertexPath = shader["vertex"].GetString();
    result.shader.fragmentPath = shader["fragment"].GetString();
    if (shader.HasMember("geometry")) {
        if (!shader["geometry"].IsString()) {
            LogError(sourcePath, "shader.geometry must be a string");
            return false;
        }
        result.shader.geometryPath = shader["geometry"].GetString();
    }

    if (!doc.HasMember("parameters")) {
        return true;
    }
    if (!doc["parameters"].IsArray()) {
        LogError(sourcePath, "parameters must be an array");
        return false;
    }

    std::unordered_set<std::string> names;
    rapidjson::SizeType index = 0;
    for (const auto& parameter : doc["parameters"].GetArray()) {
        const std::string prefix =
            "parameters[" + std::to_string(index++) + "]: ";
        if (!parameter.IsObject() ||
            !parameter.HasMember("name") || !parameter["name"].IsString() ||
            !parameter.HasMember("type") || !parameter["type"].IsString() ||
            !parameter.HasMember("value")) {
            LogError(sourcePath,
                prefix + "name, type and value are required");
            return false;
        }

        const std::string name = parameter["name"].GetString();
        const std::string type = parameter["type"].GetString();
        const auto& value = parameter["value"];
        if (name.empty() || !names.insert(name).second) {
            LogError(sourcePath, prefix + "parameter name is empty or duplicated");
            return false;
        }

        if (type == "int" && value.IsInt()) {
            result.parameters.push_back({name, value.GetInt()});
        } else if (type == "float" && value.IsNumber()) {
            result.parameters.push_back({name, value.GetFloat()});
        } else if (type == "vec2" && IsNumberArray(value, 2)) {
            result.parameters.push_back({name, glm::vec2{
                value[0].GetFloat(), value[1].GetFloat()}});
        } else if (type == "vec3" && IsNumberArray(value, 3)) {
            result.parameters.push_back({name, glm::vec3{
                value[0].GetFloat(), value[1].GetFloat(),
                value[2].GetFloat()}});
        } else if (type == "texture2D" && value.IsString()) {
            TextureAssetDesc texture;
            texture.path = value.GetString();
            std::string optionError;
            if (!ParseTextureOptions(parameter, texture, optionError)) {
                LogError(sourcePath, prefix + optionError);
                return false;
            }
            result.parameters.push_back({name, std::move(texture)});
        } else {
            LogError(sourcePath,
                prefix + "unknown type or value does not match type '" +
                type + "'");
            return false;
        }
    }
    return true;
}

std::shared_ptr<Material> MaterialAssetLoader::CreateMaterial(
    const MaterialDesc& desc) const {
    auto shader = CreateShader(desc.shader);
    if (!shader) {
        return nullptr;
    }

    struct LoadedTexture {
        std::string name;
        std::shared_ptr<Texture> texture;
    };
    std::vector<LoadedTexture> loadedTextures;
    for (const auto& parameter : desc.parameters) {
        const auto* textureDesc =
            std::get_if<TextureAssetDesc>(&parameter.value);
        if (textureDesc == nullptr) continue;

        auto texture = m_textureAssetLoader.LoadTexture(*textureDesc);
        if (!texture) {
            std::cout << "MaterialAssetLoader: failed to load texture "
                      << textureDesc->path << '\n';
            return nullptr;
        }
        loadedTextures.push_back({parameter.name, std::move(texture)});
    }

    auto material = std::make_shared<Material>(std::move(shader));
    material->SetSurfaceMode(desc.render.surface);
    material->SetRenderState(desc.render.state);
    material->SetDefaultRenderOrder(desc.render.defaultRenderOrder);
    material->SetParam("uAlphaMasked",
        desc.render.surface == SurfaceMode::Masked ? 1 : 0);
    material->SetParam("uAlphaCutoff", desc.render.alphaCutoff);
    for (const auto& parameter : desc.parameters) {
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (!std::is_same_v<T, TextureAssetDesc>) {
                material->SetParam(parameter.name, value);
            }
        }, parameter.value);
    }
    for (auto& loaded : loadedTextures) {
        material->SetTexture(
            std::move(loaded.name), std::move(loaded.texture));
    }
    return material->IsValid() ? std::move(material) : nullptr;
}

std::shared_ptr<ShaderProgram> MaterialAssetLoader::CreateShader(
    const ShaderAssetDesc& desc) const {
    ShaderAssetDesc key{
        m_fileSystem.NormalizeAssetPath(desc.vertexPath),
        m_fileSystem.NormalizeAssetPath(desc.fragmentPath),
        desc.geometryPath.empty()
            ? std::string{}
            : m_fileSystem.NormalizeAssetPath(desc.geometryPath)};
    if (key.vertexPath.empty() || key.fragmentPath.empty() ||
        (!desc.geometryPath.empty() && key.geometryPath.empty())) {
        std::cout << "MaterialAssetLoader: shader asset path is invalid\n";
        return nullptr;
    }

    const auto cached = m_shaderCache.find(key);
    if (cached != m_shaderCache.end()) {
        if (auto shader = cached->second.lock()) {
            return shader;
        }
        m_shaderCache.erase(cached);
    }

    const std::string vertex =
        m_fileSystem.LoadAssetTextFile(key.vertexPath);
    const std::string fragment =
        m_fileSystem.LoadAssetTextFile(key.fragmentPath);
    const std::string geometry = key.geometryPath.empty()
        ? std::string{}
        : m_fileSystem.LoadAssetTextFile(key.geometryPath);
    if (vertex.empty() || fragment.empty() ||
        (!key.geometryPath.empty() && geometry.empty())) {
        std::cout << "MaterialAssetLoader: shader source is empty\n";
        return nullptr;
    }

    auto shader = m_renderDevice.CreateShaderProgram(vertex, fragment, geometry);
    if (shader) {
        m_shaderCache.insert_or_assign(std::move(key), shader);
    }
    return shader;
}

void MaterialAssetLoader::PruneExpiredCaches() const {
    EraseExpired(m_materialCache);
    EraseExpired(m_shaderCache);
}

void MaterialAssetLoader::LogError(
    const std::string& sourcePath,
    const std::string& error) const {
    std::cout << "MaterialAssetLoader: " << sourcePath
              << ": " << error << '\n';
}

} // namespace eng
