#ifndef O_OPENGL_SHADER_PROGRAM
#define O_OPENGL_SHADER_PROGRAM

#include "Renderer/ShaderProgram.h"

#include <unordered_map>
#include <string>
#include <glad/glad.h>

namespace eng {

class OpenGLShaderProgram final : public ShaderProgram {
public:
    OpenGLShaderProgram(
        const std::string& vertexCode,
        const std::string& fragmentCode,
        const std::string& geometryCode = {});
    ~OpenGLShaderProgram() override;

    OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
    OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;

    void Bind() const override;
    void SetBool(const std::string& name, bool value) const override;
    void SetInt(const std::string& name, int value) const override;
    void SetFloat(const std::string& name, float value) const override;
    void SetMat4f(const std::string& name, const glm::mat4& mat) const override;
    void SetMat3f(const std::string& name, const glm::mat3& mat) const override;
    void SetVec4(const std::string& name, const glm::vec4& vec) const override;
    void SetVec4(const std::string& name, float x, float y, float z, float w) const override;
    void SetVec3(const std::string& name, const glm::vec3& vec) const override;
    void SetVec3(const std::string& name, float x, float y, float z) const override;
    void SetVec2(const std::string& name, const glm::vec2& vec) const override;
    void SetTexture(
        const std::string& name,
        const Texture& texture,
        uint32_t unit) const override;

    bool IsValid() const override;


private:
    GLint GetUniformLocation(const std::string& name) const;
    GLuint CreateShader(const std::string& code, GLenum type);
    GLuint CreateProgram(GLuint vertex, GLuint fragment, GLuint geometry);

    GLuint m_shaderProgram = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
};

} // namespace eng

#endif
