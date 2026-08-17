#ifndef O_RENDER_QUEUE
#define O_RENDER_QUEUE

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/mat4x4.hpp>

namespace eng {

class Material;
class Mesh;
class ShaderProgram;
struct RenderCommand {
    RenderCommand(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, 
        const glm::mat4& modelMatrix);

    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 modelMatrix;
};

struct CameraData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};

class RenderQueue {
public:

    bool BeginView(const CameraData& camera);
    void Submit(RenderCommand command);
    void EndView();
    void Execute();
    void Clear();



    // Call this after changing graphics state outside RenderQueue.
    void InvalidateStateCache();

private:
    struct QueuedCommand {
        RenderCommand command;
        uint64_t materialRevision;
    };

    struct RenderView {
        CameraData camera;
        std::vector<QueuedCommand> commands;
    };

    struct ShaderState {
        std::weak_ptr<ShaderProgram> shader;
        std::weak_ptr<Material> material;
        uint64_t materialRevision = 0;
    };

    ShaderState& GetShaderState(const std::shared_ptr<ShaderProgram>& shader);
    static bool IsValid(const RenderCommand& command);

    std::vector<ShaderState> m_shaderStates;
    std::weak_ptr<ShaderProgram> m_currentShader;
    std::weak_ptr<Material> m_currentTextureMaterial;
    uint64_t m_currentTextureMaterialRevision = 0;
    std::vector<std::unique_ptr<RenderView>> m_views;
    RenderView* m_currentView = nullptr;
};

} // namespace eng

#endif
