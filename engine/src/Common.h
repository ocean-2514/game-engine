#ifndef O_COMMON
#define O_COMMON

#include <glm/glm.hpp>

namespace eng
{
    
struct CameraData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};


} // namespace eng


#endif // O_COMMON