#ifndef O_OPENGL_RENDER_DEVICE
#define O_OPENGL_RENDER_DEVICE

#include "Renderer/RenderDevice.h"

#include <glad/glad.h>

namespace eng {

class OpenGLRenderDevice final : public RenderDevice {
public:
    bool Init(const Window& window) override;
    void Clear(const ClearDesc& desc) override;
    std::shared_ptr<ShaderProgram> CreateShaderProgram(
        const std::string& vertexCode,
        const std::string& fragmentCode,
        const std::string& geometryCode = {}) override;
    std::shared_ptr<Mesh> CreateMesh(
        const VertexLayout& layout,
        const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices = {}) override;
    std::shared_ptr<Texture> CreateTexture(
        const TextureDesc& textureDesc,
        const SamplerDesc& samplerDesc,
        const void* pixels,
        std::size_t byteCount) override;
    
    void SetDepthState(const DepthState& state) override;
    void SetBlendState(const BlendState& state) override;
    void SetRasterizerState(const RasterizerState& state) override;
    void SetRenderState(const RenderState& state) override;

};

} // namespace eng

#endif
