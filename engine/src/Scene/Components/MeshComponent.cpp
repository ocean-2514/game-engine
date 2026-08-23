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

    RenderCommand command{
        m_mesh,
        m_material,
        GetOwner()->GetWorldTransform()
    };
    command.phase = m_renderPhaseOverride.value_or(
        m_material->GetRenderPhase());
    command.renderOrder = m_renderOrderOverride.value_or(
        m_material->GetDefaultRenderOrder());
    queue.Submit(std::move(command));
}

void MeshComponent::SetRenderOrder(int32_t order) {
    m_renderOrderOverride = order;
}

void MeshComponent::ClearRenderOrderOverride() {
    m_renderOrderOverride.reset();
}

void MeshComponent::SetRenderPhase(RenderPhase phase) {
    m_renderPhaseOverride = phase;
}

void MeshComponent::ClearRenderPhaseOverride() {
    m_renderPhaseOverride.reset();
}

bool MeshComponent::IsValid() const {
    return GetOwner() && IsAlive() && 
        m_mesh && m_material &&
        m_mesh->IsValid() && m_material->IsValid();
}

    
} // namespace eng
