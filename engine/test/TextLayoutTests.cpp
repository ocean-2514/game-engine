#include "UI/TextLayout.h"
#include "UI/Utf8.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

using namespace eng;
namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Near(float actual, float expected, const char* message) {
    Check(std::abs(actual - expected) < 0.001f, message);
}
struct Provider : GlyphProvider {
    bool failFont = false, failGlyph = false, badMetrics = false, failSpace = false;
    float bearing = 1;
    float lineHeight = 20;
    uint32_t requestedSize = 0;
    std::optional<FontMetrics> GetFontMetrics(uint32_t size) override {
        requestedSize = size;
        if (failFont) return std::nullopt;
        const float s = size / 16.0f;
        return FontMetrics{12*s, -4*s, lineHeight*s};
    }
    GlyphId ResolveGlyph(char32_t cp) override {
        return {static_cast<uint32_t>(cp), cp == U'?' || cp == 0xFFFD};
    }
    std::optional<GlyphMetrics> GetGlyphMetrics(uint32_t index, uint32_t size) override {
        if (failGlyph || (failSpace && index == U' ')) return std::nullopt;
        const float s = size / 16.0f;
        GlyphMetrics g;
        g.advanceX = (index == U' ' ? 5 : 10) * s;
        if (badMetrics) g.advanceX = std::numeric_limits<float>::quiet_NaN();
        g.bearingX = bearing*s; g.bearingY = 10*s;
        g.width = index == U' ' ? 0 : 8; g.height = index == U' ' ? 0 : 12;
        g.hasBitmap = index != U' ';
        return g;
    }
    float GetKerning(uint32_t left, uint32_t right, uint32_t size) override {
        return left == U'A' && right == U'V' ? -2.0f * size / 16.0f : 0;
    }
};
TextLayoutResult Layout(std::string_view text, Provider& provider, float width = 0,
    TextWrap wrap = TextWrap::Word) {
    TextLayoutParams p;
    p.text = text; p.boxSize.x = width; p.wrap = wrap;
    return TextLayout::Build(p, provider);
}
std::u32string Visible(const TextLayoutResult& r) {
    std::u32string out;
    for (const auto& g : r.glyphs) if (g.codePoint != U' ' && g.codePoint != U'\t') out += g.codePoint;
    return out;
}
void Words() {
    Provider p;
    auto r = Layout("Hello",p);
    Check(r.valid && r.lines.size() == 1 && r.glyphs.size() == 5, "final word is emitted");
    Near(r.measuredSize.x,50,"single word width"); Near(r.measuredSize.y,16,"ascent/descent line box");
    Near(r.glyphs[0].position.x,1,"bearing"); Near(r.glyphs[0].position.y,2,"baseline minus bearing");
    r = Layout("Hello world",p,65);
    Check(r.lines.size() == 2 && Visible(r) == U"Helloworld", "word wrap preserves all letters");
    Near(r.lines[0].width,50,"old line excludes pending word");
    Check(r.lines[1].sourceByteOffset == 6, "wrapped word source offset");
    Near(r.glyphs[r.lines[1].firstGlyph].position.x,1,"wrapped word rebases to line origin");
    r = Layout("abcdef",p,25);
    Check(r.lines.size() == 3 && Visible(r) == U"abcdef", "long word character fallback");
    Check(r.lines[1].sourceByteOffset == 2 && r.lines[2].sourceByteOffset == 4,"long word offsets");
    for (const auto& line : r.lines) {
        Near(line.width,20,"long word line width");
        Near(r.glyphs[line.firstGlyph].position.x,1,"long word line origin");
    }
    r = Layout("ab cd ",p,45);
    Check(r.lines.size() == 1 && r.lines[0].glyphCount == 5,"overflowing space doesn't move a word");
    Near(r.measuredSize.x,45,"trailing whitespace excluded");
    r = Layout("aa bb cc ",p,50);
    Check(r.lines.size() == 2 && r.lines[1].sourceByteOffset == 6,"multiple word boundaries");
    Check(Layout("   ",p,5).lines.size() == 1,"whitespace doesn't generate phantom wrapped lines");
    r = Layout("a  b",p,100);
    Check(r.glyphs.size() == 4,"internal whitespace retained");
}
void BreaksAndAlignment() {
    Provider f;
    auto r = Layout("Hi\r\n\rthere\n",f);
    Check(r.lines.size() == 3 && Visible(r) == U"Hithere", "CRLF, CR, trailing newline");
    Check(r.lines[0].sourceByteOffset == 0 && r.lines[1].sourceByteOffset == 4
        && r.lines[2].sourceByteOffset == 5,"hard and empty line byte offsets");
    Check(r.lines[1].glyphCount == 0,"empty hard line preserved");
    Check(Layout("\n\n",f).lines.size() == 2,"explicit empty lines");
    Check(Layout("a\n",f).lines.size() == 1,"no trailing phantom line");
    r = Layout("A",f,1,TextWrap::Character);
    Check(r.lines.size() == 1 && r.glyphs.size() == 1,"over-wide glyph makes progress without extra line");
    r = Layout("AB",f,10,TextWrap::Character);
    Check(r.lines.size() == 2 && r.lines[1].sourceByteOffset == 1,"character source offset");
    r = Layout("a\n  b",f,100);
    Near(r.glyphs.back().position.x,11,"hard break preserves indentation");
    TextLayoutParams p; p.text="AB"; p.boxSize={40,40}; p.align=TextAlign::Center;
    p.verticalAlign=TextVerticalAlign::Middle;
    r=TextLayout::Build(p,f);
    Near(r.glyphs[0].position.x,11,"center alignment"); Near(r.lines[0].baseline,24,"middle alignment");
    p.align=TextAlign::Right; p.verticalAlign=TextVerticalAlign::Bottom;
    r=TextLayout::Build(p,f);
    Near(r.glyphs[0].position.x,21,"right alignment"); Near(r.lines[0].baseline,36,"bottom alignment");
    p.boxSize={0,0}; r=TextLayout::Build(p,f);
    Near(r.glyphs[0].position.x,1,"unbounded right alignment"); Near(r.lines[0].baseline,12,"unbounded vertical alignment");
    p.text="AB\nA"; p.align=TextAlign::Center; r=TextLayout::Build(p,f);
    Near(r.glyphs.back().position.x,6,"unbounded block aligns against max content width");
}
void KerningScaleAndTabs() {
    Provider f;
    auto r=Layout("AV",f,18);
    Check(r.lines.size()==1,"kerning affects wrap width"); Near(r.lines[0].width,18,"kerning width");
    Near(r.glyphs[1].position.x,9,"kerning affects glyph position");
    r=Layout("AV",f,10,TextWrap::Character);
    Check(r.lines.size()==2,"kerned pair wraps"); Near(r.lines[1].width,10,"cross-line kerning excluded");
    Near(r.glyphs[1].position.x,1,"cross-line kerning excluded from origin");
    TextLayoutParams p; p.text="AB"; p.contentScale=2;
    r=TextLayout::Build(p,f);
    Check(r.rasterPixelSize==32,"DPI raster size"); Near(r.measuredSize.x,20,"logical width at DPI 2");
    Near(r.glyphs[1].position.x,22,"physical glyph position at DPI 2");
    p.contentScale=1; p.pixelSize=16.6f; TextLayout::Build(p,f); Check(f.requestedSize==17,"round raster size");
    p.pixelSize=16; p.boxSize.x=33; p.align=TextAlign::Center; p.pixelSnap=false;
    f.bearing=1.25f; r=TextLayout::Build(p,f); Near(r.glyphs[0].position.x,7.75f,"unsnapped position");
    p.pixelSnap=true; r=TextLayout::Build(p,f); Near(r.glyphs[0].position.x,8,"pixel snap");
    f.bearing=1;
    r=Layout("A\tB",f);
    Check(r.glyphs.size()==3 && r.glyphs[1].size==glm::vec2(0),"tab is non-drawing layout item");
    Near(r.lines[0].width,40,"tab is four spaces"); Near(r.glyphs.back().position.x,31,"tab advance");
    r=Layout("A\tB",f,20);
    Check(r.lines.size()==2 && Visible(r)==U"AB", "tab participates in wrapping");
    f.failSpace=true; r=Layout("A\tB",f); Near(r.lines[0].width,52,"tab fallback when space unavailable");
}
void Utf8AndControls() {
    Provider f;
    // Avoid char8_t literals so string_view remains a view of UTF-8 bytes.
    const std::string chinese="\xE4\xB8\xAD\xE6\x96\x87";
    auto r=Layout(chinese,f,10);
    Check(r.lines.size()==2 && r.lines[1].sourceByteOffset==3,"CJK wrap and multibyte offsets");
    r=Layout("\xEF\xBB\xBF" "A\x01" "B",f);
    Check(Visible(r)==U"AB" && r.glyphs.front().sourceByteOffset==3,"BOM and C0 controls");
    const std::string malformed="\xE2" "A";
    r=Layout(malformed,f);
    Check(Visible(r)==U"\uFFFDA" && r.missingGlyphCount==1,"bad UTF-8 replaced, ASCII survives");
    std::size_t offset=0; utf8::CodePoint cp;
    Check(utf8::DecodeNext(malformed,offset,cp) && !cp.valid && offset==1,"bad prefix consumes one byte");
    Check(utf8::DecodeNext(malformed,offset,cp) && cp.value==U'A' && cp.byteOffset==1,"ASCII recovery");
    const std::string truncated="\xE2\x82"; offset=0;
    Check(utf8::DecodeNext(truncated,offset,cp) && !cp.valid && offset==2,"truncated prefix consumed together");
    const std::string valid="A\xC2\xA2\xE4\xB8\xAD\xF0\x9F\x98\x80";
    offset=0;
    for (const auto expected : {U'A',U'\u00A2',U'\u4E2D',U'\U0001F600'})
        Check(utf8::DecodeNext(valid,offset,cp) && cp.valid && cp.value==expected,"valid UTF-8 1/2/3/4 byte decoding");
    Check(!utf8::DecodeNext(valid,offset,cp),"end of input");
    for (const std::string invalid : {"\xC0\xAF", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\x80"}) {
        offset=0;
        while (utf8::DecodeNext(invalid,offset,cp)) Check(!cp.valid,"invalid Unicode scalar rejected");
        Check(offset==invalid.size(),"invalid UTF-8 always progresses");
    }
    const std::string punctuation="\xE4\xB8\xAD\xEF\xBC\x88\xE6\x96\x87\xEF\xBC\x89";
    r=Layout(punctuation,f,30);
    Check(r.lines.size()==2 && r.lines[0].glyphCount==1 && r.lines[1].glyphCount==3,"CJK parentheses stay together when they fit");
}
void ParametersAndFailures() {
    Provider f;
    TextLayoutParams p; p.text="A\tB";
    p.pixelSize=std::numeric_limits<float>::quiet_NaN(); p.contentScale=0; p.lineSpacing=-1; p.tabSize=0;
    auto r=TextLayout::Build(p,f);
    Check(r.valid && r.rasterPixelSize==16,"invalid style uses defaults"); Near(r.lines[0].width,25,"tab size clamped above zero");
    p.pixelSize=std::numeric_limits<float>::max(); p.contentScale=std::numeric_limits<float>::max();
    p.boxSize={std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()};
    r=TextLayout::Build(p,f); Check(r.valid && r.rasterPixelSize==256,"large product clamps before integer conversion");
    p={}; p.text="?"; f.failGlyph=true;
    r=TextLayout::Build(p,f); Check(r.valid && r.missingGlyphCount==1 && r.lines.size()==1,"missing mapping and metrics counted once");
    f.failGlyph=false; f.badMetrics=true;
    Check(!TextLayout::Build(p,f).valid,"NaN provider metrics rejected");
    f.badMetrics=false; f.failFont=true;
    Check(!TextLayout::Build(p,f).valid,"font metric failure");
    p.text=""; Check(TextLayout::Build(p,f).valid,"empty text needs no font metrics");
    f.failFont=false; f.lineHeight=0; p.text="A\nB";
    r=TextLayout::Build(p,f); Near(r.lines[1].baseline,28,"line height falls back to ascent plus descent");
}
void RandomizedInvariants() {
    Provider f;
    std::mt19937 rng(12345);
    constexpr std::string_view alphabet="ab AV \t\n\r";
    for (int iteration=0; iteration<600; ++iteration) {
        std::string text;
        const auto count=rng()%100;
        std::u32string expected;
        for (unsigned i=0; i<count; ++i) {
            const char c=alphabet[rng()%alphabet.size()]; text+=c;
            if (c!=' ' && c!='\t' && c!='\n' && c!='\r') expected+=static_cast<char32_t>(c);
        }
        for (auto wrap : {TextWrap::Word,TextWrap::Character,TextWrap::None}) {
            const auto r=Layout(text,f,static_cast<float>(1+rng()%70),wrap);
            Check(r.valid && Visible(r)==expected,"random layout neither loses nor duplicates letters");
            uint32_t next=0;
            for (const auto& line:r.lines) {
                Check(line.firstGlyph==next && line.glyphCount<=r.glyphs.size()-next,"line ranges partition glyphs");
                next+=line.glyphCount;
                if (line.glyphCount) Check(line.sourceByteOffset==r.glyphs[line.firstGlyph].sourceByteOffset,"line begins at correct source byte");
            }
            Check(next==r.glyphs.size(),"all glyphs belong to a line");
            for (const auto& g:r.glyphs) Check(std::isfinite(g.position.x) && std::isfinite(g.position.y),"finite glyph positions");
        }
    }
}
}
int main() {
    try {
        Words(); BreaksAndAlignment(); KerningScaleAndTabs(); Utf8AndControls();
        ParametersAndFailures(); RandomizedInvariants();
        std::cout<<"Text layout tests passed\n"; return 0;
    } catch (const std::exception& e) {
        std::cerr<<"Text layout tests failed: "<<e.what()<<'\n'; return 1;
    }
}
