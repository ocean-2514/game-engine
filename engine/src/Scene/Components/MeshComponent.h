#ifndef O_MESH_COMPONENT
#define O_MESH_COMPONENT

#include "Scene/Component.h"
#include <memory>

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

protected:
    void OnRender(RenderQueue& queue) override;

private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
};





} // namespace eng


#endif // O_MESH_COMPONENT
