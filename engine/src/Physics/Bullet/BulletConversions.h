#ifndef O_BULLET_CONVERSIONS
#define O_BULLET_CONVERSIONS

#include <LinearMath/btVector3.h>
#include <glm/glm.hpp>

namespace eng
{
    
inline btVector3 ToBtVector3(const glm::vec3& v) {
    return { v.x, v.y, v.z };
}

inline glm::vec3 ToGlmVec3(const btVector3& v) {
    return { v.x(), v.y(), v.z() };
}

} // namespace eng


#endif // O_BULLET_CONVERSIONS