#ifndef O_MATERIAL
#define O_MATERIAL

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include <variant>

#include <glm/glm.hpp>

namespace eng {

class ShaderProgram;
class RenderQueue;
class Texture;

class Material
{
public:
    Material() = default;
    explicit Material(std::shared_ptr<ShaderProgram> shaderProgram);

    void SetShaderProgram(std::shared_ptr<ShaderProgram> shaderProgram);
    void SetParam(std::string name, int value);
    void SetParam(std::string name, float value);
    void SetParam(std::string name, const glm::vec3& value);
    void SetParam(std::string name, const glm::vec2& value);
    void SetTexture(std::string name, std::shared_ptr<Texture> texture);
    void Bind() const;
    bool IsValid() const;

private:
    using ParameterValue = std::variant<int, float, glm::vec3, glm::vec2>;

    void SetParamValue(std::string name, ParameterValue value);
    void ApplyParameters() const;
    bool HasTextures() const;
    void IncrementRevision();
    uint64_t GetRevision() const;
    const std::shared_ptr<ShaderProgram>& GetShaderProgram() const;

    std::shared_ptr<ShaderProgram> m_shaderProgram;
    std::unordered_map<std::string, ParameterValue> m_params;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    uint64_t m_revision = 1;

    friend class RenderQueue;
};


    
} // namespace eng



#endif
