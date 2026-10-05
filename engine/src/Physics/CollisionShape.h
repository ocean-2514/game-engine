#ifndef O_COLLISION_SHAPE
#define O_COLLISION_SHAPE

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <variant>

namespace eng
{
    
struct BoxShapeDesc {
    glm::vec3 halfExtents{0.5f};
};

struct SphereShapeDesc {
    float radius = 0.5f;
};

struct CapsuleShapeDesc {
    float radius = 0.5f;
    float height = 1.0f; // 不含两端半球
};

struct ConvexHullShapeDesc {
    std::vector<glm::vec3> points;
};

struct TriangleMeshShapeDesc {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};

using CollisionShapeDesc = std::variant<
    BoxShapeDesc,
    SphereShapeDesc,
    CapsuleShapeDesc,
    ConvexHullShapeDesc,
    TriangleMeshShapeDesc>;


    
} // namespace eng


#endif // O_COLLISION_SHAPE
