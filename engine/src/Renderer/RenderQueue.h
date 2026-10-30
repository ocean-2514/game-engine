#ifndef O_RENDER_QUEUE
#define O_RENDER_QUEUE

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/mat4x4.hpp>

#include "Renderer/RenderState.h"
#include "Renderer/RenderCommand.h"

namespace eng {

class Material;
class ShaderProgram;
class RenderDevice;
class Mesh;
class FileSystem;
class AssetManager;

class RenderQueue {
public:
    bool Init(RenderDevice& device, 
        FileSystem& fileSystem, AssetManager& assetManager);

    bool BeginView(const RenderViewData& data);
    void Submit(RenderCommand command);
    void EndView();
    bool BeginScreenView(const ScreenViewDesc& desc);
    void SubmitScreenQuad(ScreenQuadCommand command);
    void EndScreenView();
    void Execute(RenderDevice& device, const Viewport& framebufferViewport);
    void Clear();
    void ShutDown();

    // Call this after changing graphics state outside RenderQueue.
    void InvalidateStateCache();

private:
    struct QueuedRenderCommand {
        RenderCommand command;
        uint64_t materialRevision;
    };

    using QueuedScreenQuadCommand = ScreenQuadCommand;

    struct RenderView {
        RenderViewData data;
        std::vector<QueuedRenderCommand> commands;
    };

    struct ScreenQuadView {
        ScreenViewDesc desc;
        std::vector<QueuedScreenQuadCommand> commands;
    };

    struct ShaderState {
        std::weak_ptr<ShaderProgram> shader;
        std::weak_ptr<Material> material;
        uint64_t materialRevision = 0;
    };

    void ExecuteRender(RenderDevice& device, const Viewport& framebufferViewport);
    void ExecuteScreenQuad(RenderDevice& device, const Viewport& framebufferViewport);

    ShaderState& GetShaderState(const std::shared_ptr<ShaderProgram>& shader);
    static bool IsValid(const RenderCommand& command);
    static bool IsValid(const ScreenQuadCommand& command);
    void ApplyRenderState(const RenderState& state,
        RenderDevice& device);

    std::vector<ShaderState> m_shaderStates;
    RenderState m_currentRenderState{};
    RenderState m_screenRenderState{};
    bool m_renderStateCacheValid = false;
    std::weak_ptr<ShaderProgram> m_currentShader;
    std::weak_ptr<Material> m_currentTextureMaterial;
    uint64_t m_currentTextureMaterialRevision = 0;

    std::vector<std::unique_ptr<RenderView>> m_renderViews;
    std::vector<std::unique_ptr<ScreenQuadView>> m_screenQuadViews;
    RenderView* m_currentRenderView = nullptr;
    ScreenQuadView* m_currentScreenQuadView = nullptr;

    std::shared_ptr<ShaderProgram> m_defaultScreenShader;
    std::shared_ptr<Texture> m_defaultTexture;
    std::shared_ptr<Mesh> m_quadMesh;
};

} // namespace eng

#endif
