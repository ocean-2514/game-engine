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

class Material
{
public:
    Material() = default;
    explicit Material(std::shared_ptr<ShaderProgram> shaderProgram);

    void SetShaderProgram(std::shared_ptr<ShaderProgram> shaderProgram);
    void SetParam(std::string name, int value);
    void SetParam(std::string name, float value);
    void SetParam(std::string name, glm::vec3 value);
    void Bind() const;
    bool IsValid() const;

private:
    using ParameterValue = std::variant<int, float, glm::vec3>;

    void SetParamValue(std::string name, ParameterValue value);
    void ApplyParameters() const;
    void IncrementRevision();
    uint64_t GetRevision() const;
    const std::shared_ptr<ShaderProgram>& GetShaderProgram() const;

    std::shared_ptr<ShaderProgram> m_shaderProgram;
    std::unordered_map<std::string, ParameterValue> m_params;
    uint64_t m_revision = 1;

    friend class RenderQueue;
};


    
} // namespace eng



#endif
