#ifndef O_MATERIAL_DESC
#define O_MATERIAL_DESC

#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <variant>
#include <glm/glm.hpp>

#include "Assets/ImageLoader.h"
#include "Renderer/TextureDesc.h"

namespace eng
{
    
struct TextureAssetDesc {
    std::string path;
    ImageLoadOptions loadOptions;
    SamplerDesc samplerDesc;
};

using MaterialParameterValue = std::variant<
    int,
    float,
    glm::vec2,
    glm::vec3,
    TextureAssetDesc>;

struct MaterialParameterDesc {
    std::string name;
    MaterialParameterValue value;
};

struct ShaderAssetDesc {
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;

    bool operator==(const ShaderAssetDesc& other) const {
        return vertexPath == other.vertexPath &&
               fragmentPath == other.fragmentPath &&
               geometryPath == other.geometryPath;
    }
};

struct MaterialDesc {
    uint32_t version = 1;
    std::string name;
    ShaderAssetDesc shader;
    std::vector<MaterialParameterDesc> parameters;
};



    
} // namespace eng


namespace std {
    template<>
    struct hash<eng::ShaderAssetDesc> {
        size_t operator()(const eng::ShaderAssetDesc& desc) const noexcept {
            // 组合三个字符串的哈希
            size_t h1 = std::hash<std::string>{}(desc.vertexPath);
            size_t h2 = std::hash<std::string>{}(desc.fragmentPath);
            size_t h3 = std::hash<std::string>{}(desc.geometryPath);
            
            size_t seed = 0;
            seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}

#endif // O_MATERIAL_DESC
