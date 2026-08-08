#ifndef O_OPENGL_SHADER_PROGRAM
#define O_OPENGL_SHADER_PROGRAM

#include "Renderer/ShaderProgram.h"

#include <unordered_map>
#include <glad/glad.h>

namespace eng {

class OpenGLShaderProgram final : public ShaderProgram {
public:
    explicit OpenGLShaderProgram(GLuint shaderProgram);
    ~OpenGLShaderProgram() override;

    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;

    void Bind() const override;
    void setBool(const std::string& name, bool value) const override;
    void setInt(const std::string& name, int value) const override;
    void setFloat(const std::string& name, float value) const override;
    void setMat4f(const std::string& name, const glm::mat4& mat) const override;
    void setMat3f(const std::string& name, const glm::mat3& mat) const override;
    void setVec4(const std::string& name, const glm::vec4& vec) const override;
    void setVec4(const std::string& name, float x, float y, float z, float w) const override;
    void setVec3(const std::string& name, const glm::vec3& vec) const override;
    void setVec3(const std::string& name, float x, float y, float z) const override;
    void setVec2(const std::string& name, const glm::vec2& vec) const override;

private:
    GLint GetUniformLocation(const std::string& name) const;

    GLuint m_shaderProgram = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
};

} // namespace eng

#endif
