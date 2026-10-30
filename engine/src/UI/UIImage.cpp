#include "UI/UIImage.h"

#include <cmath>

namespace eng
{

UIImage::UIImage(std::shared_ptr<Texture> texture) 
    : m_texture(std::move(texture)) {}
    
void UIImage::SetTexture(std::shared_ptr<Texture> texture) {
    m_texture = std::move(texture);
}

std::shared_ptr<Texture> UIImage::GetTexture() const {
    return m_texture;
}

void UIImage::SetUV(const glm::vec2& bottomLeft, 
    const glm::vec2& upperRight) {
    if (!std::isfinite(bottomLeft.x) || !std::isfinite(bottomLeft.y)
        || !std::isfinite(upperRight.x) || !std::isfinite(upperRight.y)) return;
    
    m_lowerLeftUV = bottomLeft;
    m_upperRightUV = upperRight;
}

glm::vec2 UIImage::GetLowerLeftUV() const {
    return m_lowerLeftUV;
}

glm::vec2 UIImage::GetUpperRightUV() const {
    return m_upperRightUV;
}

void UIImage::SetTint(const glm::vec4& tint) {
    if (!std::isfinite(tint.r) || !std::isfinite(tint.b)
        || !std::isfinite(tint.g) || !std::isfinite(tint.a)) return;
    m_tint = tint;
}

glm::vec4 UIImage::GetTint() const {
    return m_tint;
}

void UIImage::OnSubmit(UIDrawList& drawList) const {
    const auto rect = GetArrangedRect();
    if (rect.width <= 0.0f || rect.height <= 0.0f) return;
    ScreenQuadCommand cmd{
        .texture = m_texture,
        .modelMatrix = GetModelMatrix(),
        .lowerLeftUV = m_lowerLeftUV,
        .upperRightUV = m_upperRightUV,
        .color = m_tint,
        .renderOrder = GetRenderOrder()
    };
    drawList.push_back(std::move(cmd));
}

    
} // namespace eng
