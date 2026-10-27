#ifndef O_SPRITE_COMPONENT
#define O_SPRITE_COMPONENT

#include "Scene/Component.h"

#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

namespace eng {

class Texture;
class ShaderProgram;

// A scene-owned sprite in logical screen coordinates. Its size and position
// come from the owning GameObject's 2D transform.
class SpriteComponent final : public Component {
public:
    ENG_COMPONENT_TYPE(SpriteComponent);

    explicit SpriteComponent(std::shared_ptr<Texture> texture = {});

    void SetTexture(std::shared_ptr<Texture> texture);
    const std::shared_ptr<Texture>& GetTexture() const;
    void SetShader(std::shared_ptr<ShaderProgram> shader);
    const std::shared_ptr<ShaderProgram>& GetShader() const;
    bool SetUVRect(const glm::vec2& lowerLeft, const glm::vec2& upperRight);
    bool SetColor(const glm::vec4& color);
    void SetRenderOrder(int32_t order);
    int32_t GetRenderOrder() const;
    bool IsValid() const;

protected:
    void OnRender2D(RenderQueue& queue) override;

private:
    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<ShaderProgram> m_shader;
    glm::vec2 m_lowerLeftUV{0.0f};
    glm::vec2 m_upperRightUV{1.0f};
    glm::vec4 m_color{1.0f};
    int32_t m_renderOrder = 0;
};

} // namespace eng

#endif // O_SPRITE_COMPONENT
