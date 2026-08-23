#ifndef O_RENDER_DEVICE
#define O_RENDER_DEVICE

#include <memory>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Graphics/Clear.h"
#include "Renderer/TextureDesc.h"
#include "Renderer/RenderState.h"

namespace eng {

class ShaderProgram;
class Texture;
class Mesh;
class Window;
struct VertexLayout;

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual bool Init(const Window& window) = 0;
    virtual void Clear(const ClearDesc& desc) = 0;
    virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
        const std::string& vertexCode,
        const std::string& fragmentCode,
        const std::string& geometryCode = {}) = 0;
    virtual std::shared_ptr<Mesh> CreateMesh(
        const VertexLayout& layout,
        const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices = {}) = 0;
    // The device must copy or consume pixel data before this call returns;
    // it must not retain the borrowed pixels pointer.
    virtual std::shared_ptr<Texture> CreateTexture(
        const TextureDesc& textureDesc,
        const SamplerDesc& samplerDesc,
        const void* pixels,
        std::size_t byteCount) = 0;
    
    virtual void SetDepthState(const DepthState& state) = 0;
    virtual void SetBlendState(const BlendState& state) = 0;
    virtual void SetRasterizerState(const RasterizerState& state) = 0;
    virtual void SetRenderState(const RenderState& state) = 0;

    static std::unique_ptr<RenderDevice> Create();
};

} // namespace eng

#endif
