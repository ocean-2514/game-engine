#ifndef O_COMMON
#define O_COMMON

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace eng
{
    
struct CameraData {
    glm::vec3 position{0.0f};
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};

struct DirectionalLightData
{
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float intensity = 1.0f;

    glm::vec3 color{1.0f};
    float padding = 0.0f;
};

struct PointLightData
{
    glm::vec3 position{0.0f};
    float range = 10.0f;

    glm::vec3 color{1.0f};
    float intensity = 1.0f;
};

struct SpotLightData
{
    glm::vec3 position{0.0f};
    float range = 10.0f;

    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float intensity = 1.0f;

    glm::vec3 color{1.0f};
    float innerConeCos = 0.9f;

    float outerConeCos = 0.8f;
};

struct LightingData
{
    glm::vec3 ambientColor{0.15f};
    float ambientIntensity = 1.0f;

    std::vector<DirectionalLightData> directionalLights;
    std::vector<PointLightData> pointLights;
    std::vector<SpotLightData> spotLights;
};

struct LightingLimits
{
    static constexpr uint32_t MaxDirectionalLights = 2;
    static constexpr uint32_t MaxPointLights = 16;
    static constexpr uint32_t MaxSpotLights = 8;
};

} // namespace eng


#endif // O_COMMON
