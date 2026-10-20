#include "Scene/Components/AudioListenerComponent.h"
#include "Scene/Scene.h"
#include "Audio/AudioManager.h"

namespace eng
{

bool AudioListenerComponent::PushTransform() const {
    if (!m_audioManager || !m_owner) return false;
    return m_audioManager->SetListener(AudioListenerState{
        .position = GetPosition(),
        .forward = m_owner->GetWorldForward(),
        .up = m_owner->GetWorldUpward()
    });
}
    
void AudioListenerComponent::OnAttach(Scene& scene) {
    m_audioManager = scene.GetAudioManager();
}

void AudioListenerComponent::OnDetach(Scene& scene) {
    if (scene.GetAudioListener() == this) scene.SetAudioListener(nullptr);
    m_audioManager = nullptr;
}


} // namespace eng
