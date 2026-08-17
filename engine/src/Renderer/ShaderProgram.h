#ifndef O_SHADER
#define O_SHADER

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace eng {
    
class Texture;
    
class ShaderProgram {
public:
    virtual ~ShaderProgram() = default;
    
    virtual void Bind() const = 0;
    
    virtual void SetBool(const std::string& name, bool value) const = 0;
    virtual void SetInt(const std::string& name, int value) const = 0;
    virtual void SetFloat(const std::string& name, float value) const = 0;
    virtual void SetMat4f(const std::string& name, const glm::mat4& mat) const = 0;
    virtual void SetMat3f(const std::string& name, const glm::mat3& mat) const = 0;
    virtual void SetVec4(const std::string& name, const glm::vec4& vec) const = 0;
    virtual void SetVec4(const std::string& name, float x, float y, float z, float w) const = 0;
    virtual void SetVec3(const std::string& name, const glm::vec3& vec) const = 0;
    virtual void SetVec3(const std::string& name, float x, float y, float z) const = 0;
    virtual void SetVec2(const std::string& name, const glm::vec2& vec) const = 0;
    virtual void SetTexture(const std::string& name, const Texture& texture,
        uint32_t unit) const = 0;
    
    virtual bool IsValid() const = 0;
};


} // namespace eng


#endif
