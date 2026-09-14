#ifndef O_RENDER_QUEUE
#define O_RENDER_QUEUE

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/mat4x4.hpp>

#include "Common.h"
#include "Renderer/RenderState.h"

namespace eng {

class Material;
class Mesh;
class ShaderProgram;
class RenderDevice;
struct SkeletonPose;

struct RenderCommand {
    RenderCommand(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, 
        const glm::mat4& modelMatrix);

    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 modelMatrix;
    std::shared_ptr<const SkeletonPose> skeletonPose;
    RenderPhase phase = RenderPhase::Opaque;
    int32_t renderOrder = 0;
};

struct RenderViewData
{
    CameraData camera;
    LightingData lighting;
};

class RenderQueue {
public:

    bool BeginView(const RenderViewData& camera);
    void Submit(RenderCommand command);
    void EndView();
    void Execute(RenderDevice& device);
    void Clear();

    // Call this after changing graphics state outside RenderQueue.
    void InvalidateStateCache();

private:
    struct QueuedCommand {
        RenderCommand command;
        uint64_t materialRevision;
    };

    struct RenderView {
        RenderViewData data;
        std::vector<QueuedCommand> commands;
    };

    struct ShaderState {
        std::weak_ptr<ShaderProgram> shader;
        std::weak_ptr<Material> material;
        uint64_t materialRevision = 0;
    };

    ShaderState& GetShaderState(const std::shared_ptr<ShaderProgram>& shader);
    static bool IsValid(const RenderCommand& command);
    void ApplyRenderState(const RenderState& state,
        RenderDevice& device);

    std::vector<ShaderState> m_shaderStates;
    RenderState m_currentRenderState{};
    bool m_renderStateCacheValid = false;
    std::weak_ptr<ShaderProgram> m_currentShader;
    std::weak_ptr<Material> m_currentTextureMaterial;
    uint64_t m_currentTextureMaterialRevision = 0;
    std::vector<std::unique_ptr<RenderView>> m_views;
    RenderView* m_currentView = nullptr;

};

} // namespace eng

#endif
