#include "Scene/Components/MeshComponent.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/RenderQueue.h"
#include "Scene/GameObject.h"

namespace eng {
    
MeshComponent::MeshComponent(
    std::shared_ptr<Mesh> mesh,
    std::shared_ptr<Material> material)
    : m_mesh(std::move(mesh)),
      m_material(std::move(material)) {}

void MeshComponent::OnRender(RenderQueue& queue) {
    if (!IsValid()) return;

    queue.Submit(RenderCommand{
        m_mesh,
        m_material,
        GetOwner()->GetWorldTransform()
    });
}

bool MeshComponent::IsValid() const {
    return GetOwner() && IsAlive() && 
        m_mesh && m_material &&
        m_mesh->IsValid() && m_material->IsValid();
}

    
} // namespace eng
