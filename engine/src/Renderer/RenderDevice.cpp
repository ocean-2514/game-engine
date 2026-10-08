#include "Renderer/RenderDevice.h"
#include "Renderer/OpenGL/OpenGLRenderDevice.h"
#include "Renderer/MeshFactory.h"

namespace eng {

std::shared_ptr<Mesh> RenderDevice::CreateMesh(
    const MeshShapeDesc& desc) {
    return MeshFactory::CreateMesh(*this, desc);
}

std::unique_ptr<RenderDevice> RenderDevice::Create() {
    return std::make_unique<OpenGLRenderDevice>();
}

} // namespace eng
