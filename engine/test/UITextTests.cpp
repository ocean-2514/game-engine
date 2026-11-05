#include "UI/UIText.h"
#include "UI/UIContext.h"
#include "Renderer/Texture.h"
#include "Renderer/ShaderProgram.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace eng;
namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Near(float actual, float expected, const char* message) { Check(std::abs(actual-expected)<0.001f,message); }
struct TestShader : ShaderProgram {
    bool valid = true;
    void Bind() const override {}
    void SetBool(const std::string&, bool) const override {}
    void SetInt(const std::string&, int) const override {}
    void SetFloat(const std::string&, float) const override {}
    void SetMat4f(const std::string&, const glm::mat4&) const override {}
    void SetMat3f(const std::string&, const glm::mat3&) const override {}
    void SetVec4(const std::string&, const glm::vec4&) const override {}
    void SetVec4(const std::string&, float,float,float,float) const override {}
    void SetVec3(const std::string&, const glm::vec3&) const override {}
    void SetVec3(const std::string&, float,float,float) const override {}
    void SetVec2(const std::string&, const glm::vec2&) const override {}
    void SetTexture(const std::string&, const Texture&, uint32_t) const override {}
    bool IsValid() const override { return valid; }
};
struct TestTexture : Texture {
    bool valid = true;
    bool IsValid() const override { return valid; }
    bool Update(uint32_t,uint32_t,uint32_t,uint32_t,const void*,std::size_t) override { return true; }
};
struct Source : GlyphRenderSource {
    int layouts = 0, textureRequests = 0;
    float advance = 8;
    uint32_t size = 0;
    bool failRender = false;
    std::string requests;
    std::shared_ptr<TestShader> shader = std::make_shared<TestShader>();
    std::shared_ptr<TestTexture> texture = std::make_shared<TestTexture>();
    std::optional<FontMetrics> GetFontMetrics(uint32_t pixels) override {
        ++layouts; size=pixels; const float s=pixels/16.0f;
        return FontMetrics{12*s,-4*s,20*s};
    }
    GlyphId ResolveGlyph(char32_t cp) override { return {static_cast<uint32_t>(cp),false}; }
    std::optional<GlyphMetrics> GetGlyphMetrics(uint32_t id,uint32_t pixels) override {
        const float s=pixels/16.0f;
        GlyphMetrics m;
        m.advanceX=advance*s; m.bearingX=2*s; m.bearingY=10*s;
        m.width=id==U' '?0:static_cast<uint32_t>(8*s);
        m.height=id==U' '?0:static_cast<uint32_t>(12*s); m.hasBitmap=id!=U' ';
        return m;
    }
    float GetKerning(uint32_t,uint32_t,uint32_t) override { return 0; }
    std::optional<GlyphRenderData> GetGlyphRenderData(uint32_t id,uint32_t pixels) override {
        requests+='R'; if (failRender) return std::nullopt;
        const auto m=*GetGlyphMetrics(id,pixels);
        return GlyphRenderData{0,{0.2f,0.3f},{0.6f,0.9f},m.width,m.height};
    }
    std::shared_ptr<Texture> GetGlyphTexture(int) override { ++textureRequests; requests+='T'; return texture; }
    std::shared_ptr<ShaderProgram> GetGlyphShader() const override { return shader; }
};
struct TextProbe : UIText {
    using UIText::UIText;
    UIDrawList Draw() const { UIDrawList list; OnSubmit(list); return list; }
};
void PainterCachingAndGeometry() {
    auto font=std::make_shared<Source>();
    TextPainter p; p.SetSource(font); p.SetText("A B");
    UIRect box{10.25f,20.25f,100,50};
    Check(p.UpdateLayout(box,2) && font->layouts==1,"initial layout");
    Check(!p.UpdateLayout(box,2),"clean layout reused");
    UIDrawList commands; p.AppendQuads(commands,box,7);
    Check(commands.size()==2 && font->requests=="RRT" && font->textureRequests==1,
        "resolve all glyphs before one page upload; spaces do not draw");
    const auto& cmd=commands[0];
    Check(cmd.texture==font->texture && cmd.shader==font->shader && cmd.renderOrder==7,"command resources and order");
    Near(cmd.modelMatrix[0][0],8,"glyph logical width"); Near(cmd.modelMatrix[1][1],12,"glyph logical height");
    Near(cmd.modelMatrix[3][0],16.5f,"absolute physical x snap plus half width");
    Near(cmd.modelMatrix[3][1],28.5f,"absolute physical y snap plus half height");
    Check(cmd.lowerLeftUV==glm::vec2(0.2f,0.3f) && cmd.upperRightUV==glm::vec2(0.6f,0.9f),"atlas UV");
    box.x+=5; Check(!p.UpdateLayout(box,2),"position-only change doesn't reflow");
    commands.clear(); p.AppendQuads(commands,box,0,ScreenOrigin::BottomLeft);
    Near(commands[0].modelMatrix[1][1],-12,"BottomLeft keeps glyph upright");
    Near(commands[0].modelMatrix[3][1],62.5f,"BottomLeft converts top-down layout from box top");
    p.SetColor({0.2f,0.3f,0.4f,0.5f}); Check(!p.UpdateLayout(box,2),"colour is paint-only");
    commands.clear(); p.AppendQuads(commands,box,0);
    Check(commands[0].color==p.GetStyle().color,"new colour used without layout");
    auto style=p.GetStyle(); style.color=glm::vec4(0.5f); p.SetStyle(style);
    Check(!p.UpdateLayout(box,2),"SetStyle colour-only does not reflow");
    p.SetText("AB"); commands.clear(); p.AppendQuads(commands,box,0);
    Check(commands.empty(),"dirty layout isn't drawn");
    Check(p.UpdateLayout(box,2) && font->layouts==2,"text change rebuilds");
    auto other=std::make_shared<Source>(); other->advance=12; p.SetSource(other);
    Check(p.UpdateLayout(box,2) && other->layouts==1,"source change rebuilds"); Near(p.GetMeasuredSize().x,24,"new source metrics");
    p.Invalidate(); Check(p.GetSource()==other && p.GetText()=="AB","Invalidate preserves inputs");
    Check(p.UpdateLayout(box,2),"explicit invalidation rebuilds");
    box.width+=10; Check(p.UpdateLayout(box,2),"box width rebuilds");
    commands.clear(); p.AppendQuads(commands,{box.x,box.y,box.width+1,box.height},0);
    Check(commands.empty(),"mismatched box doesn't draw stale layout");
    p.SetSource(nullptr); Check(p.UpdateLayout(box,2) && !p.GetLayout().valid,"null font clears layout");
    Check(!p.UpdateLayout(box,2),"null source has stable cache");
}
void PainterSafety() {
    auto source=std::make_shared<Source>(); TextPainter p; p.SetSource(source); p.SetText("AB");
    UIRect box{0,0,100,50};
    Check(p.UpdateLayout(box,0) && source->size==16,"invalid scale uses one for both layout and draw");
    UIDrawList list; p.AppendQuads(list,box,0); Check(list.size()==2,"sanitized scale draws");
    TextStyle bad; bad.pixelSize=std::numeric_limits<float>::quiet_NaN(); bad.lineSpacing=-1;
    bad.tabSize=0; bad.color.x=std::numeric_limits<float>::infinity();
    p.SetStyle(bad);
    Check(p.GetStyle().pixelSize==16 && p.GetStyle().lineSpacing==1 && p.GetStyle().tabSize==1
        && p.GetStyle().color==glm::vec4(1),"style sanitization");
    p.UpdateLayout(box,1);
    source->shader->valid=false; list.clear(); p.AppendQuads(list,box,0);
    Check(list.empty(),"invalid shader skipped"); source->shader->valid=true;
    source->texture->valid=false; p.AppendQuads(list,box,0); Check(list.empty(),"invalid texture skipped");
    source->texture->valid=true; source->failRender=true; p.AppendQuads(list,box,0); Check(list.empty(),"failed glyph skipped");
    source->failRender=false;
    for (UIRect invalid : {UIRect{0,0,0,20},UIRect{0,0,20,0},UIRect{0,0,-1,20},
        UIRect{std::numeric_limits<float>::quiet_NaN(),0,20,20}}) {
        p.UpdateLayout(invalid,1); p.AppendQuads(list,invalid,0); Check(list.empty(),"zero/nonfinite box not drawn");
    }
    p.SetText(""); p.UpdateLayout(box,1); p.AppendQuads(list,box,0); Check(list.empty(),"empty text");
}
void UITextIntegration() {
    auto font=std::make_shared<Source>();
    TextProbe defaultText; TextProbe supplied(font);
    Check(!defaultText.IsHitTestVisible() && !supplied.IsHitTestVisible(),"both constructors ignore hit testing");
    UIContext context({200,100,ScreenOrigin::TopLeft,{0,0,400,200}});
    auto node=std::make_unique<TextProbe>(font); auto* text=node.get();
    text->SetRect({10,20,100,50}); text->SetText("AB"); context.AddRoot(std::move(node));
    context.Layout(); Check(font->size==32 && font->layouts==1,"context DPI propagated");
    Check(text->Draw().size()==2,"UIText submits glyph commands");
    text->SetText("ABC"); context.Layout();
    Check(font->layouts==2 && text->Draw().size()==3,"text setter invalidates element layout");
    text->SetPixelSize(20); context.Layout(); Check(font->size==40,"size setter invalidates element");
    text->SetAlign(TextAlign::Center); context.Layout(); Check(text->GetLayout().glyphs[0].position.x>2,"align setter");
    text->SetVerticalAlign(TextVerticalAlign::Bottom); context.Layout(); Check(text->GetLayout().glyphs[0].position.y>4,"vertical align setter");
    text->SetWrap(TextWrap::Character); context.Layout();
    auto style=text->GetStyle(); style.lineSpacing=2; text->SetStyle(style); context.Layout();
    const int before=font->layouts;
    text->SetColor({1,0,0,1}); context.Layout(); Check(font->layouts==before,"colour doesn't schedule remeasurement");
    text->SetRect({15,25,100,50}); context.Layout(); Check(font->layouts==before,"move doesn't reflow");
    context.SetView({200,100,ScreenOrigin::BottomLeft,{0,0,200,100}}); context.Layout();
    Check(font->size==20,"view change updates DPI"); Check(text->Draw()[0].modelMatrix[1][1]<0,"context origin propagated");
    auto replacement=std::make_shared<Source>(); text->SetSource(replacement); context.Layout();
    Check(replacement->layouts==1,"source setter invalidates UIText");
    text->SetRect({0,0,0,20}); context.Layout(); Check(text->Draw().empty(),"zero element doesn't draw");
    text->SetSource(nullptr); context.Layout(); Check(!text->GetLayout().valid,"clearing source");
    context.SetView({0,0,ScreenOrigin::TopLeft,{0,0,0,0}}); Near(context.GetPixelScale(),1,"invalid view scale fallback");
}
}
int main() {
    try {
        PainterCachingAndGeometry(); PainterSafety(); UITextIntegration();
        std::cout<<"UIText tests passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<"UIText tests failed: "<<e.what()<<'\n'; return 1; }
}
