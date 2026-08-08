#ifndef O_RENDER_DEVICE
#define O_RENDER_DEVICE

#include <memory>
#include <string>

namespace eng {

class ShaderProgram;
class Window;

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual bool Init(const Window& window) = 0;
    virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& geometryPath = {}) = 0;

    static std::unique_ptr<RenderDevice> Create();
};

} // namespace eng

#endif
