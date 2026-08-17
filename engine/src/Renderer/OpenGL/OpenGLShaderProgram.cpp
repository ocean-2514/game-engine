#include "Renderer/OpenGL/OpenGLShaderProgram.h"
#include "Renderer/OpenGL/OpenGLTexture.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace eng {

OpenGLShaderProgram::OpenGLShaderProgram(
    const std::string& vertexCode,
    const std::string& fragmentCode,
    const std::string& geometryCode) {
    const GLuint vertex = CreateShader(vertexCode, GL_VERTEX_SHADER);
    const GLuint fragment = CreateShader(fragmentCode, GL_FRAGMENT_SHADER);
    const GLuint geometry = geometryCode.empty()
        ? 0
        : CreateShader(geometryCode, GL_GEOMETRY_SHADER);

    const bool geometryFailed = !geometryCode.empty() && geometry == 0;
    m_shaderProgram = 0;
    if (vertex != 0 && fragment != 0 && !geometryFailed) {
        m_shaderProgram = CreateProgram(vertex, fragment, geometry);
    }

    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    if (geometry != 0) glDeleteShader(geometry);
}

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

GLuint OpenGLShaderProgram::CreateShader(const std::string& code, GLenum type) {
    const char* source = code.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        char infoLog[1024]{};
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cout << "OpenGLRenderDevice::CreateShader: compile failed: \n"
            << infoLog << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint OpenGLShaderProgram::CreateProgram(GLuint vertex,
    GLuint fragment, GLuint geometry) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (geometry != 0) {
        glAttachShader(program, geometry);
    }
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        char infoLog[1024]{};
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cout << "OpenGLRenderDevice::CreateProgram: link failed:\n"
                  << infoLog << '\n';
        glDeleteProgram(program);
        return 0;
    }

    return program;
}



void OpenGLShaderProgram::SetBool(const std::string& name, bool value) const {
    glUniform1i(GetUniformLocation(name), static_cast<GLint>(value));
}

void OpenGLShaderProgram::SetInt(const std::string& name, int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

void OpenGLShaderProgram::SetFloat(const std::string& name, float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void OpenGLShaderProgram::SetMat4f(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void OpenGLShaderProgram::SetMat3f(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void OpenGLShaderProgram::SetVec4(const std::string& name, const glm::vec4& vec) const {
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void OpenGLShaderProgram::SetVec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(GetUniformLocation(name), x, y, z, w);
}

void OpenGLShaderProgram::SetVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void OpenGLShaderProgram::SetVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(GetUniformLocation(name), x, y, z);
}

void OpenGLShaderProgram::SetVec2(const std::string& name, const glm::vec2& vec) const {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void OpenGLShaderProgram::SetTexture(
    const std::string& name,
    const Texture& texture,
    uint32_t unit) const {
    const auto* openGLTexture = dynamic_cast<const OpenGLTexture*>(&texture);
    if (openGLTexture == nullptr || !openGLTexture->IsValid()) {
        return;
    }

    glUniform1i(GetUniformLocation(name), static_cast<GLint>(unit));
    openGLTexture->Bind(unit);
}

bool OpenGLShaderProgram::IsValid() const {
    return m_shaderProgram != 0;
}


} // namespace eng
