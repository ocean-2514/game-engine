#ifndef O_MESH_FACTORY
#define O_MESH_FACTORY

#include <memory>

#include "Graphics/VertexLayout.h"

namespace eng {

class Mesh;
class RenderDevice;

// 预制网格工厂：只负责生成顶点布局、顶点与索引数据，
// 然后调用 RenderDevice::CreateMesh 创建后端资源，不直接接触 OpenGL。
// 顶点布局固定为 position(vec3) + normal(vec3) + texCoord(vec2)，
// 数据以交错排列的 float 数组直接生成，避免中间结构体与二次拷贝。
namespace MeshFactory {

VertexLayout GetVertexLayout();

// 平面：位于 XY 平面，法线朝向 +Z。
std::shared_ptr<Mesh> CreatePlane(
    RenderDevice& device,
    float width = 1.0f,
    float height = 1.0f,
    int widthSegments = 1,
    int heightSegments = 1);

// 立方体（长方体），六个面各自带有独立法线。
std::shared_ptr<Mesh> CreateBox(
    RenderDevice& device,
    float width = 1.0f,
    float height = 1.0f,
    float depth = 1.0f);

std::shared_ptr<Mesh> CreateCube(
    RenderDevice& device,
    float size = 1.0f);

// 经纬球：widthSegments 为经线分段数，heightSegments 为纬线分段数。
std::shared_ptr<Mesh> CreateSphere(
    RenderDevice& device,
    float radius = 0.5f,
    int widthSegments = 32,
    int heightSegments = 16);

// 圆柱：沿 Y 轴，带上下底面。
std::shared_ptr<Mesh> CreateCylinder(
    RenderDevice& device,
    float radius = 0.5f,
    float height = 1.0f,
    int radialSegments = 32);

// 圆锥：沿 Y 轴，带底面。
std::shared_ptr<Mesh> CreateCone(
    RenderDevice& device,
    float radius = 0.5f,
    float height = 1.0f,
    int radialSegments = 32);

} // namespace MeshFactory

} // namespace eng

#endif // O_MESH_FACTORY
