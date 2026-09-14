#ifndef O_SKINNED_MESH_COMPONENT
#define O_SKINNED_MESH_COMPONENT

#include "Renderer/Animation.h"
#include "Renderer/RenderState.h"
#include "Scene/Component.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace eng {

class Material;
class Mesh;
class GameObject;

class SkinnedMeshComponent : public Component {
public:
    ENG_COMPONENT_TYPE(SkinnedMeshComponent);

    SkinnedMeshComponent(std::shared_ptr<Mesh> mesh,
        std::shared_ptr<Material> material,
        std::shared_ptr<SkeletonPose> pose,
        GameObject* modelRoot);
    bool IsValid() const;
    void SetRenderOrder(int32_t order);
    void ClearRenderOrderOverride();
    void SetRenderPhase(RenderPhase phase);
    void ClearRenderPhaseOverride();

protected:
    void OnRender(RenderQueue& queue) override;

private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<SkeletonPose> m_pose;
    GameObject* m_modelRoot = nullptr;
    std::optional<int32_t> m_renderOrderOverride;
    std::optional<RenderPhase> m_renderPhaseOverride;
};

} // namespace eng

#endif // O_SKINNED_MESH_COMPONENT
