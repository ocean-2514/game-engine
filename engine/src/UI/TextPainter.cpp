#include "UI/TextPainter.h"
#include "UI/TextLayout.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Texture.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace eng {
namespace {
float CleanScale(float value) { return std::isfinite(value) && value > 0 ? value : 1.0f; }
bool Finite(glm::vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
bool Finite(glm::vec4 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
}
bool ValidBox(const UIRect& box) {
    return std::isfinite(box.x) && std::isfinite(box.y)
        && std::isfinite(box.width) && std::isfinite(box.height)
        && box.width >= 0 && box.height >= 0;
}
TextStyle CleanStyle(TextStyle style) {
    if (!std::isfinite(style.pixelSize) || style.pixelSize <= 0) style.pixelSize = 16;
    if (!std::isfinite(style.lineSpacing) || style.lineSpacing <= 0) style.lineSpacing = 1;
    if (!Finite(style.color)) style.color = glm::vec4(1);
    style.tabSize = std::clamp(style.tabSize, 1u, 32u);
    if (style.align != TextAlign::Left && style.align != TextAlign::Center && style.align != TextAlign::Right)
        style.align = TextAlign::Left;
    if (style.verticalAlign != TextVerticalAlign::Top && style.verticalAlign != TextVerticalAlign::Middle
        && style.verticalAlign != TextVerticalAlign::Bottom) style.verticalAlign = TextVerticalAlign::Top;
    if (style.wrap != TextWrap::None && style.wrap != TextWrap::Word && style.wrap != TextWrap::Character)
        style.wrap = TextWrap::Word;
    return style;
}
}

void TextPainter::SetSource(std::shared_ptr<GlyphRenderSource> source) {
    if (m_source == source) return;
    m_source = std::move(source);
    Invalidate();
}
const std::shared_ptr<GlyphRenderSource>& TextPainter::GetSource() const { return m_source; }
void TextPainter::SetText(std::string text) {
    if (m_text == text) return;
    m_text = std::move(text);
    Invalidate();
}
const std::string& TextPainter::GetText() const { return m_text; }
void TextPainter::SetStyle(const TextStyle& style) {
    const auto clean = CleanStyle(style);
    auto oldGeometry = m_style;
    oldGeometry.color = clean.color;
    if (!(oldGeometry == clean)) Invalidate();
    m_style = clean; // Colour is paint-only; no glyph measurement is needed.
}
const TextStyle& TextPainter::GetStyle() const { return m_style; }
void TextPainter::SetColor(const glm::vec4& color) {
    m_style.color = Finite(color) ? color : glm::vec4(1);
}
void TextPainter::SetPixelSize(float size) { auto s = m_style; s.pixelSize = size; SetStyle(s); }
void TextPainter::SetAlign(TextAlign align) { auto s = m_style; s.align = align; SetStyle(s); }
void TextPainter::SetVerticalAlign(TextVerticalAlign align) { auto s = m_style; s.verticalAlign = align; SetStyle(s); }
void TextPainter::SetWrap(TextWrap wrap) { auto s = m_style; s.wrap = wrap; SetStyle(s); }

void TextPainter::Invalidate() {
    m_layout = {};
    m_dirty = true;
}
bool TextPainter::UpdateLayout(const UIRect& box, float pixelScale) {
    pixelScale = CleanScale(pixelScale);
    const glm::vec2 boxSize{box.width, box.height};
    if (!ValidBox(box)) { Invalidate(); return true; }
    if (!m_dirty && boxSize == m_lastBoxSize && pixelScale == m_lastPixelScale) return false;
    m_layout = {};
    if (m_source) {
        TextLayoutParams p;
        p.text = m_text; p.boxSize = boxSize; p.pixelSize = m_style.pixelSize;
        p.contentScale = pixelScale; p.align = m_style.align; p.verticalAlign = m_style.verticalAlign;
        p.wrap = m_style.wrap; p.lineSpacing = m_style.lineSpacing;
        p.pixelSnap = m_style.pixelSnap; p.tabSize = m_style.tabSize;
        m_layout = TextLayout::Build(p, *m_source);
    }
    m_lastBoxSize = boxSize;
    m_lastPixelScale = pixelScale;
    m_dirty = false;
    return true;
}
const TextLayoutResult& TextPainter::GetLayout() const { return m_layout; }
glm::vec2 TextPainter::GetMeasuredSize() const { return m_layout.measuredSize; }

void TextPainter::AppendQuads(UIDrawList& drawList, const UIRect& box, int32_t renderOrder,
    ScreenOrigin origin) const {
    if (m_dirty || !m_layout.valid || !m_source || !ValidBox(box)
        || box.width <= 0 || box.height <= 0 || glm::vec2(box.width, box.height) != m_lastBoxSize
        || (origin != ScreenOrigin::TopLeft && origin != ScreenOrigin::BottomLeft)) return;
    auto shader = m_source->GetGlyphShader();
    if (!shader || !shader->IsValid()) return;

    // Resolve all glyphs first. Otherwise each new glyph could cause an entire
    // dirty atlas page to be uploaded again while processing this same string.
    struct Ready { const GlyphPlacement* glyph; GlyphRenderData data; };
    std::vector<Ready> ready;
    for (const auto& glyph : m_layout.glyphs) {
        if (glyph.size.x <= 0 || glyph.size.y <= 0) continue;
        auto data = m_source->GetGlyphRenderData(glyph.glyphIndex, m_layout.rasterPixelSize);
        if (data && data->page >= 0 && data->width > 0 && data->height > 0
            && Finite(data->lowerLeftUV) && Finite(data->upperRightUV)) ready.push_back({&glyph, *data});
    }

    std::unordered_map<int, std::shared_ptr<Texture>> textures;
    const double scale = m_lastPixelScale;
    for (const auto& entry : ready) {
        auto [it, inserted] = textures.try_emplace(entry.data.page);
        if (inserted) it->second = m_source->GetGlyphTexture(entry.data.page);
        if (!it->second || !it->second->IsValid()) continue;
        const auto& glyph = *entry.glyph;
        double x = box.x + glyph.position.x / scale;
        double y = origin == ScreenOrigin::TopLeft
            ? box.y + glyph.position.y / scale
            : static_cast<double>(box.y) + box.height - glyph.position.y / scale;
        if (m_style.pixelSnap) {
            x = std::round(x * scale) / scale;
            y = std::round(y * scale) / scale;
        }
        const double w = glyph.size.x / scale;
        const double h = glyph.size.y / scale * (origin == ScreenOrigin::TopLeft ? 1 : -1);
        const double cx = x + w / 2, cy = y + h / 2;
        const auto fits = [](double v) {
            return std::isfinite(v) && std::abs(v) <= std::numeric_limits<float>::max();
        };
        if (!fits(cx) || !fits(cy) || !fits(w) || !fits(h)) continue;
        // The shared screen mesh has v=1 at local y=-0.5. Negative height in
        // BottomLeft keeps that corner at the glyph's visual top without
        // changing the atlas's bottom-left UV convention.
        ScreenQuadCommand command;
        command.modelMatrix = glm::mat4(1);
        command.modelMatrix[0][0] = static_cast<float>(w);
        command.modelMatrix[1][1] = static_cast<float>(h);
        command.modelMatrix[3][0] = static_cast<float>(cx);
        command.modelMatrix[3][1] = static_cast<float>(cy);
        command.texture = it->second;
        command.shader = shader;
        command.lowerLeftUV = entry.data.lowerLeftUV;
        command.upperRightUV = entry.data.upperRightUV;
        command.color = m_style.color;
        command.renderOrder = renderOrder;
        drawList.push_back(std::move(command));
    }
}
} // namespace eng
