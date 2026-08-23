#include "Renderer/RenderState.h"

namespace eng
{
    
bool DepthState::operator==(const DepthState& other) const {
    return depthTestEnable == other.depthTestEnable
        && depthWriteEnable == other.depthWriteEnable
        && depthCompareOp == other.depthCompareOp;
}

bool DepthState::operator!=(const DepthState& other) const {
    return !operator==(other);
}

bool BlendState::operator==(const BlendState& other) const {
    return blendEnable == other.blendEnable
        && sourceColor == other.sourceColor
        && destinationColor == other.destinationColor
        && colorOperation == other.colorOperation;
}

bool BlendState::operator!=(const BlendState& other) const {
    return !operator==(other);
}

bool RasterizerState::operator==(const RasterizerState& other) const {
    return cullMode == other.cullMode
        && frontFace == other.frontFace;
}

bool RasterizerState::operator!=(const RasterizerState& other) const {
    return !operator==(other);
}

bool RenderState::operator==(const RenderState& other) const {
    return depth == other.depth
        && blend == other.blend
        && rasterizer == other.rasterizer;
}

bool RenderState::operator!=(const RenderState& other) const {
    return !operator==(other);
}

RenderState MakeRenderState(SurfaceMode mode) {
    RenderState state;
    switch (mode) {
        case SurfaceMode::Opaque:
        case SurfaceMode::Masked:
            state.blend.blendEnable = false;
            state.depth.depthWriteEnable = true;
            break;
        case SurfaceMode::Transparent:
            state.blend.blendEnable = true;
            state.depth.depthWriteEnable = false;
            break;
    }
    return state;
}

RenderPhase GetDefaultRenderPhase(SurfaceMode mode) {
    switch (mode) {
        case SurfaceMode::Opaque:      return RenderPhase::Opaque;
        case SurfaceMode::Masked:      return RenderPhase::AlphaTest;
        case SurfaceMode::Transparent: return RenderPhase::Transparent;
    }
    return RenderPhase::Opaque;
}

    
} // namespace eng
