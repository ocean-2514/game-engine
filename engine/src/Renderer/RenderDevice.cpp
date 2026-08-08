#include "Renderer/RenderDevice.h"
#include "Renderer/OpenGL/OpenGLRenderDevice.h"

namespace eng {

std::unique_ptr<RenderDevice> RenderDevice::Create() {
    return std::make_unique<OpenGLRenderDevice>();
}

} // namespace eng
