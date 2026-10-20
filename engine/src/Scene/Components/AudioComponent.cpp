#include "Scene/Components/AudioComponent.h"
#include "Audio/AudioManager.h"
#include "Scene/Scene.h"
#include <cmath>
#include <iostream>

namespace eng
{
    
AudioComponent::AudioComponent(const AudioSourceDesc& desc)
    : m_desc(desc) {}

bool AudioComponent::Play() {
    if (!m_audioManager) return false;
    return m_audioManager->Play(m_handle);
}

bool AudioComponent::Pause() {
    if (!m_audioManager) return false;
    return m_audioManager->Pause(m_handle);
}

bool AudioComponent::Stop() {
    if (!m_audioManager) return false;
    return m_audioManager->Stop(m_handle);
}

bool AudioComponent::SetVolume(float linearGain) {
    if (!std::isfinite(linearGain) || linearGain < 0.0f) return false;
    if (m_audioManager && m_handle.index != InvalidAudioId &&
        !m_audioManager->SetVoiceVolume(m_handle, linearGain)) return false;
    m_desc.volume = linearGain;
    return true;
}

bool AudioComponent::PushTransform() const {
    if (!m_audioManager) return false;
    return m_audioManager->SetTransform(m_handle, GetPosition());
}

AudioPlaybackState AudioComponent::GetPlaybackState() const {
    if (!m_audioManager) return {};
    return m_audioManager->GetPlaybackState(m_handle);
}

void AudioComponent::OnAttach(Scene& scene) {
    m_audioManager = scene.GetAudioManager();
    if (!m_audioManager) return;

    m_handle = m_audioManager->CreateVoice(m_desc);
    if (m_handle.index == InvalidAudioId) {
        std::cerr << "AudioComponent: failed to create voice for '"
            << m_desc.assetPath << "'\n";
        m_audioManager = nullptr;
        return;
    }
    PushTransform();
    scene.RegisterAudio(this);
    if (m_desc.playOnAttach) {
        m_audioManager->Play(m_handle);
    }
}

void AudioComponent::OnDetach(Scene& scene) {
    scene.UnregisterAudio(this);
    if (m_audioManager && m_handle.index != InvalidAudioId) {
        m_audioManager->DestroyVoice(m_handle);
    }
    m_handle = {};
    m_audioManager = nullptr;
}




} // namespace eng
