#ifndef O_AUDIO_LISTENER_COMPONENT
#define O_AUDIO_LISTENER_COMPONENT

#include "Scene/Component.h"
#include "Audio/AudioTypes.h"

namespace eng
{

class AudioManager;
    
class AudioListenerComponent : public Component {
public:
    ENG_COMPONENT_TYPE(AudioListenerComponent);

protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;

private:
    friend class Scene;
    bool PushTransform() const;
    AudioManager* m_audioManager = nullptr;
};




} // namespace eng


#endif // O_AUDIO_LISTENER_COMPONENT
