#ifndef O_AUDIO_COMPONENT
#define O_AUDIO_COMPONENT

#include "Scene/Component.h"
#include "Audio/AudioTypes.h"
#include <glm/glm.hpp>

namespace eng
{

class AudioManager;
    
class AudioComponent : public Component {
public:
    ENG_COMPONENT_TYPE(AudioComponent);

    explicit AudioComponent(const AudioSourceDesc& desc);
    bool Play();
    bool Pause();
    bool Stop();
    bool SetVolume(float linearGain);
    bool PushTransform() const;
    AudioPlaybackState GetPlaybackState() const;

protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;

private:
    AudioSourceDesc m_desc;
    AudioVoiceHandle m_handle;
    AudioManager* m_audioManager = nullptr;
};




} // namespace eng


#endif // O_AUDIO_COMPONENT
