#ifndef O_OPENGL_RENDER_DEVICE
#define O_OPENGL_RENDER_DEVICE

#include "Renderer/RenderDevice.h"

#include <glad/glad.h>

namespace eng {

class OpenGLRenderDevice final : public RenderDevice {
public:
    bool Init(const Window& window) override;
    std::shared_ptr<ShaderProgram> CreateShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& geometryPath = {}) override;

private:
    GLuint CreateShader(const std::string& path, GLenum type) const;
    GLuint CreateProgram(GLuint vertex, GLuint fragment, GLuint geometry = 0) const;
};

} // namespace eng

#endif
