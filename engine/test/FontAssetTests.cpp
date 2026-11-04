#include "Assets/Font/Font.h"
#include "Assets/Font/FontFace.h"
#include "Assets/FontAssetLoader.h"
#include "IO/FileSystem.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Texture.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace eng;
namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct TestShader : ShaderProgram {
    void Bind() const override {}
    void SetBool(const std::string&, bool) const override {}
    void SetInt(const std::string&, int) const override {}
    void SetFloat(const std::string&, float) const override {}
    void SetMat4f(const std::string&, const glm::mat4&) const override {}
    void SetMat3f(const std::string&, const glm::mat3&) const override {}
    void SetVec4(const std::string&, const glm::vec4&) const override {}
    void SetVec4(const std::string&, float, float, float, float) const override {}
    void SetVec3(const std::string&, const glm::vec3&) const override {}
    void SetVec3(const std::string&, float, float, float) const override {}
    void SetVec2(const std::string&, const glm::vec2&) const override {}
    void SetTexture(const std::string&, const Texture&, uint32_t) const override {}
    bool IsValid() const override { return true; }
};

struct TestTexture : Texture {
    std::vector<uint8_t> bytes;
    int updates = 0;
    bool fail = false;
    TestTexture(const TextureDesc& desc, const void* pixels, std::size_t count) {
        m_width = desc.width; m_height = desc.height; m_format = desc.format;
        bytes.assign(static_cast<const uint8_t*>(pixels), static_cast<const uint8_t*>(pixels) + count);
    }
    bool IsValid() const override { return true; }
    bool Update(uint32_t, uint32_t, uint32_t, uint32_t, const void* pixels, std::size_t count) override {
        ++updates;
        if (fail) return false;
        bytes.assign(static_cast<const uint8_t*>(pixels), static_cast<const uint8_t*>(pixels) + count);
        return true;
    }
};

struct TestDevice : RenderDevice {
    int textures = 0;
    bool failCreate = false;
    bool Init(const Window&) override { return true; }
    void Clear(const ClearDesc&) override {}
    std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& vs,
        const std::string& fs, const std::string&) override {
        Check(!vs.empty() && !fs.empty(), "shader sources must exist");
        return std::make_shared<TestShader>();
    }
    std::shared_ptr<Mesh> CreateMesh(const VertexLayout&, const std::vector<float>&,
        const std::vector<uint32_t>&) override { return nullptr; }
    std::shared_ptr<Texture> CreateTexture(const TextureDesc& desc, const SamplerDesc&,
        const void* pixels, std::size_t count) override {
        ++textures;
        return failCreate ? nullptr : std::make_shared<TestTexture>(desc, pixels, count);
    }
    void SetDepthState(const DepthState&) override {}
    void SetBlendState(const BlendState&) override {}
    void SetRasterizerState(const RasterizerState&) override {}
    void SetRenderState(const RenderState&) override {}
    void SetViewport(const Viewport&) override {}
};

std::shared_ptr<FontFace> Face(const FileSystem& files) {
    auto bytes = files.LoadAssetFile("font/DejaVuSansMono.ttf");
    Check(!bytes.empty(), "missing fixture assets/font/DejaVuSansMono.ttf");
    auto face = FontFace::Load(std::move(bytes), 0, {});
    Check(face && face->IsValid(), "font fixture must load");
    return face;
}

void Faces(const FileSystem& files) {
    auto face = Face(files);
    Check(!face->GetMetrics(), "metrics unavailable before setting size");
    Check(face->SetRasterPixelSize(24) && face->GetRasterPixelSize() == 24, "current size");
    const auto metrics = face->GetMetrics();
    Check(metrics && metrics->ascender > 0 && metrics->descender < 0 && metrics->lineHeight > 0,
        "font metrics in pixels");
    Check(metrics->underlineThickness > 0, "scaled underline");
    auto a = face->LoadGlyph(face->GetGlyphIndex(U'A'), 24);
    Check(a && a->metrics.advanceX > 0 && a->metrics.advanceX < 24, "advance uses pixels, not 26.6");
    Check(a->pixels.size() == static_cast<std::size_t>(a->pitch) * a->metrics.height, "packed bitmap");
    auto small = face->LoadGlyph(face->GetGlyphIndex(U'A'), 12);
    Check(small && small->metrics.advanceX < a->metrics.advanceX, "alternating sizes");
    auto space = face->LoadGlyph(face->GetGlyphIndex(U' '), 24);
    Check(space && !space->metrics.hasBitmap && space->metrics.advanceX > 0, "space has advance only");
    Check(!face->SetRasterPixelSize(0) && !face->LoadGlyph(InvalidIndex, 24), "invalid size and glyph");
    Check(face->GetGlyphIndex(static_cast<char32_t>(0xD800)) == 0, "surrogate rejected");
    Check(!FontFace::Load({}, 0, {}) && !FontFace::Load({'b','a','d'}, 0, {}), "invalid font data");
    Check(!FontFace::Load(files.LoadAssetFile("font/DejaVuSansMono.ttf"), -1, {}), "negative face index");
}

void Atlases() {
    GlyphAtlas atlas({8, 8, 2, 1});
    std::vector<GlyphAllocation> allocations;
    for (int i = 0; i < 8; ++i) {
        auto a = atlas.Allocate(2, 2);
        Check(a.has_value(), "shelf and page allocation");
        for (const auto& b : allocations) {
            Check(a->page != b.page || a->x + a->width <= b.x || b.x + b.width <= a->x
                || a->y + a->height <= b.y || b.y + b.height <= a->y, "allocations must not overlap");
        }
        allocations.push_back(*a);
    }
    Check(atlas.GetPageCount() == 2 && allocations[2].y == 5, "new shelf before new page");
    Check(!atlas.Allocate(2, 2) && !atlas.Allocate(0, 1), "full and zero allocations");
    GlyphBitmap bitmap;
    bitmap.metrics.width = 2; bitmap.metrics.height = 2; bitmap.pitch = 3;
    bitmap.pixels = {10, 20, 99, 30, 40};
    atlas.SetPageDirty(0, false);
    Check(atlas.Write(allocations[0], bitmap) && atlas.IsPageDirty(0), "write dirties page");
    const auto& pixels = atlas.GetPagePixels(0);
    Check(pixels[1 * 8 + 1] == 30 && pixels[2 * 8 + 1] == 10 && pixels[0] == 0,
        "top-down to bottom-left with empty padding");
    atlas.SetPageDirty(0, false);
    bitmap.pixels.pop_back();
    Check(!atlas.Write(allocations[0], bitmap) && !atlas.IsPageDirty(0), "short pitched data rejected");
    bitmap.pitch = 1;
    Check(!atlas.Write(allocations[0], bitmap), "short pitch rejected");
    for (const GlyphAtlasDesc desc : {GlyphAtlasDesc{0,8,1,1}, {8,8,0,1}, {8,8,1,4},
        {8,8,1,std::numeric_limits<uint32_t>::max()}}) {
        GlyphAtlas invalid(desc);
        Check(!invalid.IsValid() && !invalid.Allocate(1,1), "invalid atlas rejected without allocating");
    }
    Check(!atlas.Allocate(std::numeric_limits<uint32_t>::max(), 1), "overflowing glyph rejected");
}

void Fonts(const FileSystem& files) {
    auto face = Face(files);
    TestDevice device;
    Font font(face, device, std::make_shared<TestShader>(), {128,128,4,1});
    const auto a = font.ResolveGlyph(U'A').index, b = font.ResolveGlyph(U'B').index;
    Check(font.GetGlyphRenderData(a, 24).has_value(), "A render data");
    Check(font.GetDiagnostics().uploadedGlyphs == 0, "CPU packing isn't a GPU upload");
    device.failCreate = true;
    Check(!font.GetGlyphTexture(0) && font.GetDiagnostics().uploadFailures == 1, "create failure");
    device.failCreate = false;
    auto texture = std::dynamic_pointer_cast<TestTexture>(font.GetGlyphTexture(0));
    Check(texture && font.GetDiagnostics().uploadedGlyphs == 1, "retry create");
    const auto oldPixels = texture->bytes;
    Check(font.GetGlyphTexture(0) == texture && texture->updates == 0, "clean page needs no upload");
    Check(font.GetGlyphRenderData(b, 24).has_value(), "B added after first upload");
    texture->fail = true;
    Check(!font.GetGlyphTexture(0) && texture->updates == 1, "failed update retains dirty state");
    texture->fail = false;
    Check(font.GetGlyphTexture(0) == texture && texture->updates == 2 && texture->bytes != oldPixels,
        "update in place and retry");
    Check(font.GetGlyphTexture(0) == texture && texture->updates == 2, "dirty resets after upload");
    Check(font.GetDiagnostics().uploadedGlyphs == 2 && font.GetDiagnostics().atlasPages == 1, "diagnostics");
    Check(!font.GetGlyphTexture(-1) && !font.GetGlyphTexture(-2) && !font.GetGlyphTexture(100), "invalid pages");
    Check(font.GetGlyphTexture(0) == texture, "invalid lookup doesn't discard pages");
    Check(!font.GetGlyphRenderData(font.ResolveGlyph(U' ').index, 24), "spaces do not allocate");
    for (int i = 0; i < 2; ++i) Check(!font.GetGlyphMetrics(InvalidIndex, 24), "failure cache");
    Check(font.GetDiagnostics().rasterizationFailures == 1, "failure counted once");
    font.ResolveGlyph(static_cast<char32_t>(0x10FFFF));
    font.ResolveGlyph(static_cast<char32_t>(0x10FFFF));
    Check(font.GetDiagnostics().missingCodePoints == 1, "missing codepoint counted once");
    Font tiny(face, device, {}, {4,4,1,1});
    Check(!tiny.GetGlyphRenderData(a,24) && !tiny.GetGlyphRenderData(a,24)
        && tiny.GetDiagnostics().atlasOverflows == 1, "permanent atlas failure counted once");
    const auto aMetrics = *font.GetGlyphMetrics(a,24);
    const auto bMetrics = *font.GetGlyphMetrics(b,24);
    Font pages(face, device, {}, {std::max(aMetrics.width, bMetrics.width) + 2,
        std::max(aMetrics.height, bMetrics.height) + 2, 8, 1});
    auto first = pages.GetGlyphRenderData(a,24);
    auto second = pages.GetGlyphRenderData(b,24);
    Check(first && second && first->page != second->page, "multipage fixture");
    auto high = pages.GetGlyphTexture(second->page);
    Check(high && pages.GetGlyphTexture(first->page) && pages.GetGlyphTexture(second->page) == high,
        "loading a lower page must not shrink the texture array");
    Font noFace({}, device, {}, {});
    Check(!noFace.GetFontMetrics(24) && !noFace.GetGlyphMetrics(0,24), "null face is safe");
}

void Loaders(const FileSystem& files) {
    TestDevice device;
    FontAssetLoader loader(files, device);
    FontAssetDesc desc; desc.path = "font/DejaVuSansMono.ttf";
    auto font = loader.LoadFont(desc);
    Check(font != nullptr, "font loader");
    auto equivalent = desc; equivalent.path = "font/../font/DejaVuSansMono.ttf";
    Check(loader.LoadFont(equivalent) == font, "normalized cache key");
    equivalent = desc; equivalent.atlas.pageWidth = 512;
    auto other = loader.LoadFont(equivalent);
    Check(other && other != font, "atlas configuration is part of cache key");
    font.reset();
    Check(loader.LoadFont(desc) != nullptr, "expired font with live shared face");
    other.reset();
    Check(loader.LoadFont(desc) != nullptr, "expired face can reload");
    equivalent = desc; equivalent.path = "scenes/scene1.json";
    Check(!loader.LoadFont(equivalent) && !loader.LoadFont(equivalent), "malformed font fails without crashing");
    equivalent = desc; equivalent.atlas.padding = std::numeric_limits<uint32_t>::max();
    Check(!loader.LoadFont(equivalent), "loader rejects invalid atlas");
    equivalent = desc; equivalent.faceIndex = -1;
    Check(!loader.LoadFont(equivalent), "loader rejects negative face");
}
}

int main() {
    try {
        const FileSystem files;
        Faces(files); Atlases(); Fonts(files); Loaders(files);
        std::cout << "Font asset tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Font asset tests failed: " << e.what() << '\n';
        return 1;
    }
}
