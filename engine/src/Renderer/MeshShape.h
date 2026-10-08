#ifndef O_MESH_SHAPE
#define O_MESH_SHAPE

#include <cstdint>
#include <vector>
#include <variant>

#include "Graphics/VertexLayout.h"

namespace eng
{

struct PlaneMeshDesc {
    float width = 1.0f;
    float height = 1.0f;
    int widthSegments = 1;
    int heightSegments = 1;
};

struct BoxMeshDesc {
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
};

struct CubeMeshDesc {
    float size = 1.0f;
};

struct SphereMeshDesc {
    float radius = 0.5f;
    int widthSegments = 32;
    int heightSegments = 16;
};  

struct CylinderMeshDesc {
    float radius = 0.5f;
    float height = 1.0f;
    int radialSegments = 32;
};

struct ConeMeshDesc {
    float radius = 0.5f;
    float height = 1.0f;
    int radialSegments = 32;
};  

struct CommonMeshDesc {
    VertexLayout layout{};
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};

using MeshShapeDesc = std::variant<
    PlaneMeshDesc, BoxMeshDesc, CubeMeshDesc,
    SphereMeshDesc, CylinderMeshDesc, ConeMeshDesc, 
    CommonMeshDesc>;

} // namespace eng


#endif // O_MESH_SHAPE
