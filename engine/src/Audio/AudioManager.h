#ifndef O_AUDIO_MANAGER
#define O_AUDIO_MANAGER

#include "Audio/AudioTypes.h"

#include <glm/glm.hpp>
#include <string>
#include <optional>
#include <memory>

namespace eng
{

class FileSystem;

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    bool Init(const FileSystem& fileSystem);
    void Shutdown();
    bool IsReady() const;
    void SetMasterVolume(float linearGain);

    AudioVoiceHandle CreateVoice(const AudioSourceDesc& desc);
    void DestroyVoice(AudioVoiceHandle handle);
    bool Play(AudioVoiceHandle handle);
    bool Pause(AudioVoiceHandle handle);
    bool Stop(AudioVoiceHandle handle);
    AudioPlaybackState GetPlaybackState(AudioVoiceHandle handle) const;
    bool SetTransform(AudioVoiceHandle handle,
        const glm::vec3& position, 
        const glm::vec3& velocity = glm::vec3{0.0f});
    bool SetVoiceVolume(AudioVoiceHandle handle, float volume);
    bool SetListener(const AudioListenerState& state); 
    void ClearListener();
    AudioVoiceHandle PlayOneShot(
        const std::string& assetPath,
        const std::optional<glm::vec3>& worldPosition = std::nullopt);
    void Update(); 

private:
    struct Impl;

    bool IsValidHandle(AudioVoiceHandle handle) const;
    std::unique_ptr<Impl> m_impl;
    const FileSystem* m_fileSystem = nullptr;
    uint32_t m_nextGeneration = 1;
};


} // namespace eng


#endif // O_AUDIO_MANAGER
