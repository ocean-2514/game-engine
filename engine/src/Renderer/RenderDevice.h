#ifndef O_RENDER_DEVICE
#define O_RENDER_DEVICE

#include <memory>
#include <cstdint>
#include <string>
#include <vector>

#include "Graphics/Clear.h"

namespace eng {

class ShaderProgram;
class Mesh;
class Window;
struct VertexLayout;

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual bool Init(const Window& window) = 0;
    virtual void Clear(const ClearDesc& desc) = 0;
    virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& geometryPath = {}) = 0;
    virtual std::shared_ptr<Mesh> CreateMesh(
        const VertexLayout& layout,
        const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices = {}) = 0;

    static std::unique_ptr<RenderDevice> Create();
};

} // namespace eng

#endif
