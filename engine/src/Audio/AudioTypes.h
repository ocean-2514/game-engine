#ifndef O_AUDIO_TYPES
#define O_AUDIO_TYPES

#include <string>
#include <limits>
#include <cstdint>
#include <glm/glm.hpp>

namespace eng
{
    
inline constexpr uint32_t InvalidAudioId = 
    std::numeric_limits<uint32_t>::max();
    
enum class AudioLoadMode { Decode, Stream };
enum class AudioPlaybackState { Stopped, Playing, Paused };

struct AudioSourceDesc {
    std::string assetPath;                 // 相对 assets 根目录
    AudioLoadMode loadMode = AudioLoadMode::Decode;
    bool spatial = true;
    bool looping = false;
    bool playOnAttach = false;
    float volume = 1.0f;                   // 线性增益，非 dB
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
};

struct AudioListenerState {
    glm::vec3 position{0.0f};           
    glm::vec3 forward{0.0f, 0.0f, -1.0f}; 
    glm::vec3 up{0.0f, 1.0f, 0.0f};    
    glm::vec3 velocity{0.0f};          
};

struct AudioVoiceHandle {
    uint32_t index = InvalidAudioId;
    uint32_t generation = 0;
};




} // namespace eng


#endif // O_AUDIO_TYPES
