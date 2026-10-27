#ifndef O_RENDER_COMMAND
#define O_RENDER_COMMAND

#include "EngineCommon.h"

#include <memory>
#include <optional>
#include <glm/glm.hpp>
#include <cstdint>

namespace eng
{

class Material;
class Mesh;
class Texture;
class ShaderProgram;
struct SkeletonPose;
    
struct RenderCommand {
    RenderCommand(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, 
        const glm::mat4& modelMatrix);

    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 modelMatrix{1.0f};
    std::shared_ptr<const SkeletonPose> skeletonPose;
    RenderPhase phase = RenderPhase::Opaque;
    int32_t renderOrder = 0;
};

struct ScreenQuadCommand {
    std::shared_ptr<Texture> texture;
    // Optional override. Must follow the screen-quad uModel/uProjection,
    // uColor, uLowerLeftUV/uUpperRightUV and uTexture uniform contract.
    std::shared_ptr<ShaderProgram> shader;
    glm::mat4 modelMatrix{1.0f};
    glm::vec2 lowerLeftUV{0.0f};
    glm::vec2 upperRightUV{1.0f};
    glm::vec4 color{1.0f};
    int32_t renderOrder = 0;
};

struct RenderViewData
{
    CameraData camera;
    LightingData lighting;
    std::optional<Viewport> viewport; // Omitted: full framebuffer viewport.
};

struct ScreenViewDesc {
    float logicalWidth = 0.0f;
    float logicalHeight = 0.0f;
    ScreenOrigin origin = ScreenOrigin::TopLeft;
    Viewport viewport;
};

} // namespace eng


#endif // O_RENDER_COMMAND
