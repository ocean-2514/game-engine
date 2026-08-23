#ifndef O_RENDER_STATE
#define O_RENDER_STATE

#include <cstdint>

namespace eng
{

enum class CompareOp {
    Less, LessEqual, Greater, GreaterEqual,
    Never, Always, Equal, NotEqual 
};
    
struct DepthState
{
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    CompareOp depthCompareOp = CompareOp::Less;

    bool operator==(const DepthState& other) const;
    bool operator!=(const DepthState& other) const;
};

enum class BlendFactor {
    Zero, One, SourceAlpha, OneMinusSourceAlpha
};

enum class BlendOp {
    Add, Subtract, ReverseSubtract, Min, Max
};


struct BlendState
{
    bool blendEnable = false;
    BlendFactor sourceColor = BlendFactor::SourceAlpha;
    BlendFactor destinationColor =
        BlendFactor::OneMinusSourceAlpha;
    BlendOp colorOperation = BlendOp::Add;

    bool operator==(const BlendState& other) const;
    bool operator!=(const BlendState& other) const;
};


enum class CullMode {
    None, Front, Back
};

enum class FrontFace {
    Clockwise, CounterClockwise   
};

struct RasterizerState
{
    CullMode cullMode = CullMode::None;
    FrontFace frontFace = FrontFace::CounterClockwise;

    bool operator==(const RasterizerState& other) const;
    bool operator!=(const RasterizerState& other) const;
};

struct RenderState
{
    DepthState depth{};
    BlendState blend{};
    RasterizerState rasterizer{};

    bool operator==(const RenderState& other) const;
    bool operator!=(const RenderState& other) const;
};

enum class SurfaceMode : uint8_t {
    Opaque,
    Masked,
    Transparent
};

enum class RenderPhase : uint8_t {
    Opaque,
    AlphaTest,
    Transparent,
    Overlay
};

RenderState MakeRenderState(SurfaceMode mode);
RenderPhase GetDefaultRenderPhase(SurfaceMode mode);



} // namespace eng


#endif // O_RENDER_STATE
