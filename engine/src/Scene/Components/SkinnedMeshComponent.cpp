#include "Scene/Components/SkinnedMeshComponent.h"

#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderQueue.h"
#include "Scene/GameObject.h"

namespace eng {

SkinnedMeshComponent::SkinnedMeshComponent(
    std::shared_ptr<Mesh> mesh,
    std::shared_ptr<Material> material,
    std::shared_ptr<SkeletonPose> pose,
    GameObject* modelRoot)
    : m_mesh(std::move(mesh)),
      m_material(std::move(material)),
      m_pose(std::move(pose)),
      m_modelRoot(modelRoot) {}

void SkinnedMeshComponent::OnRender(RenderQueue& queue) {
    if (!IsValid()) return;
    RenderCommand command{m_mesh, m_material, m_modelRoot->GetWorldTransform()};
    command.skeletonPose = m_pose;
    command.phase = m_renderPhaseOverride.value_or(m_material->GetRenderPhase());
    command.renderOrder = m_renderOrderOverride.value_or(
        m_material->GetDefaultRenderOrder());
    queue.Submit(std::move(command));
}

bool SkinnedMeshComponent::IsValid() const {
    return GetOwner() && IsAlive() && m_mesh && m_mesh->IsValid() &&
        m_material && m_material->IsValid() && m_pose &&
        m_modelRoot && m_modelRoot->IsAlive();
}

void SkinnedMeshComponent::SetRenderOrder(int32_t order) {
    m_renderOrderOverride = order;
}
void SkinnedMeshComponent::ClearRenderOrderOverride() {
    m_renderOrderOverride.reset();
}
void SkinnedMeshComponent::SetRenderPhase(RenderPhase phase) {
    m_renderPhaseOverride = phase;
}
void SkinnedMeshComponent::ClearRenderPhaseOverride() {
    m_renderPhaseOverride.reset();
}

} // namespace eng
