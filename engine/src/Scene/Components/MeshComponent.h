#ifndef O_MESH_COMPONENT
#define O_MESH_COMPONENT

#include "Scene/Component.h"
#include "Renderer/RenderState.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace eng {

class Mesh;
class Material;
    
class MeshComponent : public Component
{
public:
    ENG_COMPONENT_TYPE(MeshComponent);

    MeshComponent(std::shared_ptr<Mesh> mesh,
        std::shared_ptr<Material> material);
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
    std::optional<int32_t> m_renderOrderOverride;
    std::optional<RenderPhase> m_renderPhaseOverride;
};





} // namespace eng


#endif // O_MESH_COMPONENT
