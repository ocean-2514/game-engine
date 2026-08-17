#include "Assets/MaterialAssetLoader.h"

#include "Assets/ImageLoader.h"
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

template<typename T>
void HashCombine(std::size_t& seed, const T& value) {
    seed ^= std::hash<T>{}(value) + 0x9e3779b9u +
        (seed << 6) + (seed >> 2);
}

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
    RenderDevice& renderDevice)
    : m_fileSystem(fileSystem),
      m_renderDevice(renderDevice) {}

bool MaterialAssetLoader::TextureCacheKey::operator==(
    const TextureCacheKey& other) const {
    return path == other.path &&
        srgb == other.srgb &&
        flipVertically == other.flipVertically &&
        generateMipmaps == other.generateMipmaps &&
        sampler.addressU == other.sampler.addressU &&
        sampler.addressV == other.sampler.addressV &&
        sampler.minFilter == other.sampler.minFilter &&
        sampler.magFilter == other.sampler.magFilter &&
        sampler.mipFilter == other.sampler.mipFilter;
}

std::size_t MaterialAssetLoader::TextureCacheKeyHash::operator()(
    const TextureCacheKey& key) const noexcept {
    std::size_t seed = 0;
    HashCombine(seed, key.path);
    HashCombine(seed, key.srgb);
    HashCombine(seed, key.flipVertically);
    HashCombine(seed, key.generateMipmaps);
    HashCombine(seed, static_cast<int>(key.sampler.addressU));
    HashCombine(seed, static_cast<int>(key.sampler.addressV));
    HashCombine(seed, static_cast<int>(key.sampler.minFilter));
    HashCombine(seed, static_cast<int>(key.sampler.magFilter));
    HashCombine(seed, static_cast<int>(key.sampler.mipFilter));
    return seed;
}

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

void MaterialAssetLoader::ClearCache() {
    m_materialCache.clear();
    m_shaderCache.clear();
    m_textureCache.clear();
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

        auto texture = LoadTexture(*textureDesc);
        if (!texture) {
            std::cout << "MaterialAssetLoader: failed to load texture "
                      << textureDesc->path << '\n';
            return nullptr;
        }
        loadedTextures.push_back({parameter.name, std::move(texture)});
    }

    auto material = std::make_shared<Material>(std::move(shader));
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

std::shared_ptr<Texture> MaterialAssetLoader::LoadTexture(
    const TextureAssetDesc& desc) const {
    const std::string normalizedPath =
        m_fileSystem.NormalizeAssetPath(desc.path);
    if (normalizedPath.empty()) {
        std::cout << "MaterialAssetLoader: texture asset path is invalid: "
                  << desc.path << '\n';
        return nullptr;
    }

    TextureCacheKey key{
        normalizedPath,
        desc.loadOptions.srgb,
        desc.loadOptions.flipVertically,
        desc.loadOptions.generateMipmaps,
        desc.samplerDesc};
    const auto cached = m_textureCache.find(key);
    if (cached != m_textureCache.end()) {
        if (auto texture = cached->second.lock()) {
            return texture;
        }
        m_textureCache.erase(cached);
    }

    const auto image = LoadAssetImage(
        m_fileSystem, normalizedPath, desc.loadOptions);
    if (!image) {
        return nullptr;
    }
    auto texture = m_renderDevice.CreateTexture(
        image->textureDesc,
        desc.samplerDesc,
        image->pixels.data(),
        image->pixels.size());
    if (texture) {
        m_textureCache.insert_or_assign(std::move(key), texture);
    }
    return texture;
}

void MaterialAssetLoader::PruneExpiredCaches() const {
    EraseExpired(m_materialCache);
    EraseExpired(m_shaderCache);
    EraseExpired(m_textureCache);
}

void MaterialAssetLoader::LogError(
    const std::string& sourcePath,
    const std::string& error) const {
    std::cout << "MaterialAssetLoader: " << sourcePath
              << ": " << error << '\n';
}

} // namespace eng
