#include "Renderer/OpenGL/OpenGLRenderDevice.h"

#include "Platform/Window.h"
#include "Renderer/OpenGL/OpenGLShaderProgram.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace eng {

namespace {
const Window* g_loaderWindow = nullptr;

void* LoadOpenGLProc(const char* name) {
    return g_loaderWindow != nullptr ? g_loaderWindow->GetGraphicsProcAddress(name) : nullptr;
}
} // namespace

bool OpenGLRenderDevice::Init(const Window& window) {
    g_loaderWindow = &window;
    const bool loaded = gladLoadGLLoader(LoadOpenGLProc) != 0;
    g_loaderWindow = nullptr;

    if (!loaded) {
        std::cout << "OpenGLRenderDevice::Init: failed to initialize GLAD\n";
    }
    return loaded;
}

GLuint OpenGLRenderDevice::CreateShader(const std::string& path, GLenum type) const {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string code;
    try {
        file.open(path);
        std::stringstream stream;
        stream << file.rdbuf();
        code = stream.str();
    } catch (const std::ifstream::failure& error) {
        std::cout << "OpenGLRenderDevice::CreateShader: failed to read " << path
                  << ": " << error.what() << '\n';
        return 0;
    }

    const char* source = code.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        char infoLog[1024]{};
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cout << "OpenGLRenderDevice::CreateShader: compile failed for "
                  << path << ":\n" << infoLog << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint OpenGLRenderDevice::CreateProgram(GLuint vertex, GLuint fragment, GLuint geometry) const {
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

std::shared_ptr<ShaderProgram> OpenGLRenderDevice::CreateShaderProgram(
    const std::string& vertexPath,
    const std::string& fragmentPath,
    const std::string& geometryPath) {
    const GLuint vertex = CreateShader(vertexPath, GL_VERTEX_SHADER);
    const GLuint fragment = CreateShader(fragmentPath, GL_FRAGMENT_SHADER);
    const GLuint geometry = geometryPath.empty()
        ? 0
        : CreateShader(geometryPath, GL_GEOMETRY_SHADER);

    const bool geometryFailed = !geometryPath.empty() && geometry == 0;
    GLuint program = 0;
    if (vertex != 0 && fragment != 0 && !geometryFailed) {
        program = CreateProgram(vertex, fragment, geometry);
    }

    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    if (geometry != 0) glDeleteShader(geometry);

    return program != 0
        ? std::make_shared<OpenGLShaderProgram>(program)
        : nullptr;
}

} // namespace eng
