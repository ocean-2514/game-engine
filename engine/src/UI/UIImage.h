#ifndef O_UI_IMAGE
#define O_UI_IMAGE

#include "UI/UIElement.h"

#include <memory>

namespace eng
{
class Texture;
    
class UIImage : public UIElement
{
public:
    UIImage() = default;
    explicit UIImage(std::shared_ptr<Texture> texture);
    void SetTexture(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> GetTexture() const;
    void SetUV(const glm::vec2& bottomLeft, 
        const glm::vec2& upperRight);
    glm::vec2 GetLowerLeftUV() const;
    glm::vec2 GetlowerLeftUV() const { return GetLowerLeftUV(); } // Compatibility spelling.
    glm::vec2 GetUpperRightUV() const;
    void SetTint(const glm::vec4& tint);
    glm::vec4 GetTint() const;
    
protected:
    void OnSubmit(UIDrawList& drawList) const override;

private:
    std::shared_ptr<Texture> m_texture;
    glm::vec2 m_lowerLeftUV{0.0f};
    glm::vec2 m_upperRightUV{1.0f};
    glm::vec4 m_tint{1.0f};
};

    
} // namespace eng


#endif // O_UI_IMAGE
