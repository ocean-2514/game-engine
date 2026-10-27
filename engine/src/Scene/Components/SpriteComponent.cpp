#include "Scene/Components/SpriteComponent.h"

#include "Renderer/RenderQueue.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Texture.h"
#include "Scene/GameObject.h"

#include <cmath>
#include <utility>

namespace eng {
namespace {

bool IsFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsFinite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z) && std::isfinite(value.w);
}

} // namespace

SpriteComponent::SpriteComponent(std::shared_ptr<Texture> texture)
    : m_texture(std::move(texture)) {}

void SpriteComponent::SetTexture(std::shared_ptr<Texture> texture) {
    m_texture = std::move(texture);
}

const std::shared_ptr<Texture>& SpriteComponent::GetTexture() const {
    return m_texture;
}

void SpriteComponent::SetShader(std::shared_ptr<ShaderProgram> shader) {
    m_shader = std::move(shader);
}

const std::shared_ptr<ShaderProgram>& SpriteComponent::GetShader() const {
    return m_shader;
}

bool SpriteComponent::SetUVRect(const glm::vec2& lowerLeft,
    const glm::vec2& upperRight) {
    if (!IsFinite(lowerLeft) || !IsFinite(upperRight)) return false;
    m_lowerLeftUV = lowerLeft;
    m_upperRightUV = upperRight;
    return true;
}

bool SpriteComponent::SetColor(const glm::vec4& color) {
    if (!IsFinite(color)) return false;
    m_color = color;
    return true;
}

void SpriteComponent::SetRenderOrder(int32_t order) {
    m_renderOrder = order;
}

int32_t SpriteComponent::GetRenderOrder() const {
    return m_renderOrder;
}

bool SpriteComponent::IsValid() const {
    return m_owner && m_owner->IsAlive() && IsAlive() &&
        m_texture && m_texture->IsValid() &&
        (!m_shader || m_shader->IsValid());
}

void SpriteComponent::OnRender2D(RenderQueue& queue) {
    if (!IsValid()) return;
    ScreenQuadCommand command;
    command.texture = m_texture;
    command.shader = m_shader;
    command.modelMatrix = m_owner->GetWorldTransform2D();
    command.lowerLeftUV = m_lowerLeftUV;
    command.upperRightUV = m_upperRightUV;
    command.color = m_color;
    command.renderOrder = m_renderOrder;
    queue.SubmitScreenQuad(std::move(command));
}

} // namespace eng
