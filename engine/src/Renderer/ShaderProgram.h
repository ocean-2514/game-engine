#ifndef O_SHADER
#define O_SHADER

#include <string>
#include <glm/glm.hpp>

namespace eng {
    
    
    
class ShaderProgram {
public:
    virtual ~ShaderProgram() = default;
    
    virtual void Bind() const = 0;
    
    virtual void setBool(const std::string& name, bool value) const = 0;
    virtual void setInt(const std::string& name, int value) const = 0;
    virtual void setFloat(const std::string& name, float value) const = 0;
    virtual void setMat4f(const std::string& name, const glm::mat4& mat) const = 0;
    virtual void setMat3f(const std::string& name, const glm::mat3& mat) const = 0;
    virtual void setVec4(const std::string& name, const glm::vec4& vec) const = 0;
    virtual void setVec4(const std::string& name, float x, float y, float z, float w) const = 0;
    virtual void setVec3(const std::string& name, const glm::vec3& vec) const = 0;
    virtual void setVec3(const std::string& name, float x, float y, float z) const = 0;
    virtual void setVec2(const std::string& name, const glm::vec2& vec) const = 0;
    
};


} // namespace eng


#endif
