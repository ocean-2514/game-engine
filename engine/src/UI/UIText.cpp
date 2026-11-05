#include "UI/UIText.h"
#include "UI/UIContext.h"


namespace eng {

UIText::UIText() {
    SetHitTestVisible(false);
}

UIText::UIText(std::shared_ptr<GlyphRenderSource> source) : UIText() {
    m_painter.SetSource(std::move(source));
}

void UIText::SetSource(std::shared_ptr<GlyphRenderSource> source) {
    m_painter.SetSource(std::move(source));
    InvalidateLayout();
}

const std::shared_ptr<GlyphRenderSource>& UIText::GetSource() const {
    return m_painter.GetSource();
}

void UIText::SetText(std::string text) {
    m_painter.SetText(std::move(text));
    InvalidateLayout();
}

const std::string& UIText::GetText() const {
    return m_painter.GetText();
}

void UIText::SetStyle(const TextStyle& style) {
    m_painter.SetStyle(style);
    InvalidateLayout();
}

const TextStyle& UIText::GetStyle() const {
    return m_painter.GetStyle();
}

void UIText::SetColor(const glm::vec4& color) {
    m_painter.SetColor(color);
}

void UIText::SetPixelSize(float pixelSize) {
    m_painter.SetPixelSize(pixelSize);
    InvalidateLayout();
}

void UIText::SetAlign(TextAlign align) {
    m_painter.SetAlign(align);
    InvalidateLayout();
}

void UIText::SetVerticalAlign(TextVerticalAlign align) {
    m_painter.SetVerticalAlign(align);
    InvalidateLayout();
}

void UIText::SetWrap(TextWrap wrap) {
    m_painter.SetWrap(wrap);
    InvalidateLayout();
}

glm::vec2 UIText::GetMeasuredSize() const {
    return m_painter.GetMeasuredSize();
}

const TextLayoutResult& UIText::GetLayout() const {
    return m_painter.GetLayout();
}

void UIText::OnLayout() {
    const auto* context = GetUIContext();
    m_painter.UpdateLayout(GetArrangedRect(), context ? context->GetPixelScale() : 1.0f);
}

void UIText::OnSubmit(UIDrawList& drawList) const {
    m_painter.AppendQuads(drawList,
        GetArrangedRect(), GetRenderOrder(),
        GetUIContext() ? GetUIContext()->GetScreenOrigin() : ScreenOrigin::TopLeft);
}



} // namespace eng
