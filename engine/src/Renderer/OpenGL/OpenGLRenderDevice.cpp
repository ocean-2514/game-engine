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
} // namespace

bool OpenGLRenderDevice::Init(const Window& window) {
    g_loaderWindow = &window;
    const bool loaded = gladLoadGLLoader(LoadOpenGLProc) != 0;
    g_loaderWindow = nullptr;

    if (!loaded) {
        std::cout << "OpenGLRenderDevice::Init: failed to initialize GLAD\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    return true;
}

void OpenGLRenderDevice::Clear(const ClearDesc& desc) {
    GLbitfield mask = 0;

    if (HasClearBuffer(desc.buffers, ClearBuffer::Color)) {
        glClearColor(
            desc.color.red,
            desc.color.green,
            desc.color.blue,
            desc.color.alpha);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (HasClearBuffer(desc.buffers, ClearBuffer::Depth)) {
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


} // namespace eng
