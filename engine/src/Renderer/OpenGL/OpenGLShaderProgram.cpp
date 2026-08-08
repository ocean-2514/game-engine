#include "Renderer/OpenGL/OpenGLShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>

namespace eng {

OpenGLShaderProgram::OpenGLShaderProgram(GLuint shaderProgram)
    : m_shaderProgram(shaderProgram) {}

OpenGLShaderProgram::~OpenGLShaderProgram() {
    glDeleteProgram(m_shaderProgram);
}

void OpenGLShaderProgram::Bind() const {
    glUseProgram(m_shaderProgram);
}

GLint OpenGLShaderProgram::GetUniformLocation(const std::string& name) const {
    const auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }
    const GLint location = glGetUniformLocation(m_shaderProgram, name.c_str());
    m_uniformLocationCache.emplace(name, location);
    return location;
}

void OpenGLShaderProgram::setBool(const std::string& name, bool value) const {
    glUniform1i(GetUniformLocation(name), static_cast<GLint>(value));
}

void OpenGLShaderProgram::setInt(const std::string& name, int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

void OpenGLShaderProgram::setFloat(const std::string& name, float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void OpenGLShaderProgram::setMat4f(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void OpenGLShaderProgram::setMat3f(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void OpenGLShaderProgram::setVec4(const std::string& name, const glm::vec4& vec) const {
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void OpenGLShaderProgram::setVec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(GetUniformLocation(name), x, y, z, w);
}

void OpenGLShaderProgram::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void OpenGLShaderProgram::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(GetUniformLocation(name), x, y, z);
}

void OpenGLShaderProgram::setVec2(const std::string& name, const glm::vec2& vec) const {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

} // namespace eng
