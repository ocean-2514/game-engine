#ifndef O_BULLET_CONVERSIONS
#define O_BULLET_CONVERSIONS

#include <LinearMath/btVector3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace eng
{
    
inline btVector3 ToBtVector3(const glm::vec3& v) {
    return { v.x, v.y, v.z };
}

inline glm::vec3 ToGlmVec3(const btVector3& v) {
    return { v.x(), v.y(), v.z() };
}

inline btQuaternion ToBtQuaternion(const glm::quat& q) {
    return { q.x, q.y, q.z, q.w };
}

inline glm::quat ToGlmQuat(const btQuaternion& q) {
    return { q.w(), q.x(), q.y(), q.z() };
}

} // namespace eng


#endif // O_BULLET_CONVERSIONS
