#include "Renderer/OpenGL/OpenGLRenderDevice.h"

#include "Platform/Window.h"
#include "Renderer/OpenGL/OpenGLShaderProgram.h"
#include "Renderer/OpenGL/OpenGLMesh.h"
#include "Renderer/OpenGL/OpenGLTexture.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace eng {

namespace {
const Window* g_loaderWindow = nullptr;

void* LoadOpenGLProc(const char* name) {
    return g_loaderWindow != nullptr ? g_loaderWindow->GetGraphicsProcAddress(name) : nullptr;
}

GLboolean ToOpenGLBoolean(bool flag) {
    return flag ? GL_TRUE : GL_FALSE;
}


GLenum ToOpenGLCompareOp(CompareOp op) {
    switch (op)
    {
        case CompareOp::Less:           return GL_LESS;
        case CompareOp::LessEqual:      return GL_LEQUAL;
        case CompareOp::Greater:        return GL_GREATER;
        case CompareOp::GreaterEqual:   return GL_GEQUAL;
        case CompareOp::Never:          return GL_NEVER;
        case CompareOp::Always:         return GL_ALWAYS;
        case CompareOp::Equal:          return GL_EQUAL;
        case CompareOp::NotEqual:       return GL_NOTEQUAL;
        default:                        return GL_LESS;
    }
}

GLenum ToOpenGLBlendFactor(BlendFactor factor) {
    switch (factor)
    {
        case BlendFactor::One:                  return GL_ONE;
        case BlendFactor::Zero:                 return GL_ZERO;
        case BlendFactor::SourceAlpha:          return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSourceAlpha:  return GL_ONE_MINUS_SRC_ALPHA;
        default:                                return GL_ZERO;
    }
}

GLenum ToOpenGLBlendOp(BlendOp op) {
    switch (op)
    {
        case BlendOp::Add:              return GL_FUNC_ADD;
        case BlendOp::Subtract:         return GL_FUNC_SUBTRACT;
        case BlendOp::ReverseSubtract:  return GL_FUNC_REVERSE_SUBTRACT;
        case BlendOp::Min:              return GL_MIN;
        case BlendOp::Max:              return GL_MAX;
        default:                        return GL_FUNC_ADD;
    }
}

GLenum ToOpenGLCullMode(CullMode mode) {
    switch (mode)
    {
        case CullMode::Front:           return GL_FRONT;
        case CullMode::Back:            return GL_BACK;
        case CullMode::None:            return GL_BACK;
        default:                        return GL_BACK;
    }
}

GLenum ToOpenGLFrontFace(FrontFace frontFace) {
    switch (frontFace)
    {
        case FrontFace::Clockwise:          return GL_CW;
        case FrontFace::CounterClockwise:   return GL_CCW;
        default:                            return GL_CCW;
    }
}

} // namespace

bool OpenGLRenderDevice::Init(const Window& window) {
    g_loaderWindow = &window;
    const bool loaded = gladLoadGLLoader(LoadOpenGLProc) != 0;
    g_loaderWindow = nullptr;

    if (!loaded) {
        std::cout << "OpenGLRenderDevice::Init: failed to initialize GLAD\n";
        return false;
    }

    SetRenderState(RenderState{});

    return true;
}

void OpenGLRenderDevice::Clear(const ClearDesc& desc) {
    GLbitfield mask = 0;
    GLboolean previousDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

    if (HasClearBuffer(desc.buffers, ClearBuffer::Color)) {
        glClearColor(
            desc.color.red,
            desc.color.green,
            desc.color.blue,
            desc.color.alpha);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (HasClearBuffer(desc.buffers, ClearBuffer::Depth)) {
        glDepthMask(GL_TRUE);
        glClearDepth(static_cast<GLdouble>(desc.depth));
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (HasClearBuffer(desc.buffers, ClearBuffer::Stencil)) {
        glClearStencil(static_cast<GLint>(desc.stencil));
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    if (mask != 0) {
        glClear(mask);
    }

    if (HasClearBuffer(desc.buffers, ClearBuffer::Depth)) {
        glDepthMask(previousDepthMask);
    }
}

std::shared_ptr<ShaderProgram> OpenGLRenderDevice::CreateShaderProgram(
    const std::string& vertexCode,
    const std::string& fragmentCode,
    const std::string& geometryCode) {

    auto program = std::make_shared<OpenGLShaderProgram>(vertexCode,
        fragmentCode, geometryCode);

    return program->IsValid() ? program : nullptr;
}

std::shared_ptr<Mesh> OpenGLRenderDevice::CreateMesh(
    const VertexLayout& layout,
    const std::vector<float>& vertices,
    const std::vector<uint32_t>& indices) {
    auto mesh = std::make_shared<OpenGLMesh>(layout, vertices, indices);
    return mesh->IsValid() ? std::move(mesh) : nullptr;
}

std::shared_ptr<Texture> OpenGLRenderDevice::CreateTexture(
    const TextureDesc& textureDesc,
    const SamplerDesc& samplerDesc,
    const void* pixels,
    std::size_t byteCount) {
    auto texture = std::make_shared<OpenGLTexture>(
        textureDesc, samplerDesc, pixels, byteCount);
    return texture->IsValid() ? std::move(texture) : nullptr;
}

void OpenGLRenderDevice::SetDepthState(const DepthState& state) {
    glDepthMask(ToOpenGLBoolean(state.depthWriteEnable));
    if (!state.depthTestEnable) {
        glDisable(GL_DEPTH_TEST);
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(ToOpenGLCompareOp(state.depthCompareOp));
}

void OpenGLRenderDevice::SetBlendState(const BlendState& state) {
    if (!state.blendEnable) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(ToOpenGLBlendFactor(state.sourceColor),
        ToOpenGLBlendFactor(state.destinationColor));
    glBlendEquation(ToOpenGLBlendOp(state.colorOperation));
}

void OpenGLRenderDevice::SetRasterizerState(const RasterizerState& state) {
    if (state.cullMode == CullMode::None) {
        glDisable(GL_CULL_FACE);
        return;
    }
    glEnable(GL_CULL_FACE);
    glCullFace(ToOpenGLCullMode(state.cullMode));
    glFrontFace(ToOpenGLFrontFace(state.frontFace));
}

void OpenGLRenderDevice::SetRenderState(const RenderState& state) {
    SetDepthState(state.depth);
    SetBlendState(state.blend);
    SetRasterizerState(state.rasterizer);
}



} // namespace eng
