#include "Audio/AudioManager.h"
#include "IO/FileSystem.h"

#include <miniaudio/miniaudio.h>

#include <glm/geometric.hpp>

#include <cmath>
#include <filesystem>
#include <limits>
#include <utility>
#include <vector>

namespace eng {
namespace {

struct MaEngineDeleter {
    void operator()(ma_engine* engine) const {
        if (engine) {
            ma_engine_uninit(engine);
            delete engine;
        }
    }
};

struct MaSoundDeleter {
    void operator()(ma_sound* sound) const {
        if (sound) {
            ma_sound_uninit(sound);
            delete sound;
        }
    }
};

using MaEnginePtr = std::unique_ptr<ma_engine, MaEngineDeleter>;
using MaSoundPtr = std::unique_ptr<ma_sound, MaSoundDeleter>;

bool IsFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}

bool IsValidDesc(const AudioSourceDesc& desc) {
    return !desc.assetPath.empty() &&
        (desc.loadMode == AudioLoadMode::Decode ||
            desc.loadMode == AudioLoadMode::Stream) &&
        std::isfinite(desc.volume) && desc.volume >= 0.0f &&
        std::isfinite(desc.pitch) && desc.pitch > 0.0f &&
        std::isfinite(desc.minDistance) && desc.minDistance >= 0.0f &&
        std::isfinite(desc.maxDistance) &&
        desc.maxDistance >= desc.minDistance;
}

} // namespace

struct AudioManager::Impl {
    struct VoiceSlot {
        uint32_t generation = 0;
        MaSoundPtr sound;
        AudioPlaybackState state = AudioPlaybackState::Stopped;
        bool looping = false;
        bool oneShot = false;
        bool spatial = false;
    };

    MaEnginePtr engine;
    std::vector<VoiceSlot> voices;
    std::vector<uint32_t> freeSlots;

    VoiceSlot* Find(AudioVoiceHandle handle) {
        if (handle.index >= voices.size()) return nullptr;
        auto& slot = voices[handle.index];
        return slot.sound && slot.generation == handle.generation
            ? &slot : nullptr;
    }

    const VoiceSlot* Find(AudioVoiceHandle handle) const {
        if (handle.index >= voices.size()) return nullptr;
        const auto& slot = voices[handle.index];
        return slot.sound && slot.generation == handle.generation
            ? &slot : nullptr;
    }

    void Release(uint32_t index) {
        auto& slot = voices[index];
        slot.sound.reset(); // Must happen before uninitializing ma_engine.
        slot.state = AudioPlaybackState::Stopped;
        slot.oneShot = false;
        freeSlots.push_back(index);
    }
};

AudioManager::AudioManager() : m_impl(std::make_unique<Impl>()) {}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Init(const FileSystem& fileSystem) {
    if (IsReady()) return true;

    auto pending = std::make_unique<ma_engine>();
    if (ma_engine_init(nullptr, pending.get()) != MA_SUCCESS) {
        // The engine was never initialized; only release its allocation.
        return false;
    }
    m_impl->engine.reset(pending.release());
    m_fileSystem = &fileSystem;
    ma_engine_listener_set_enabled(m_impl->engine.get(), 0, MA_FALSE);
    return true;
}

void AudioManager::Shutdown() {
    if (!m_impl) return;
    m_impl->voices.clear();
    m_impl->freeSlots.clear();
    m_impl->engine.reset();
    m_fileSystem = nullptr;
}

bool AudioManager::IsReady() const {
    return m_impl && m_impl->engine != nullptr;
}

void AudioManager::SetMasterVolume(float linearGain) {
    if (IsReady() && std::isfinite(linearGain) && linearGain >= 0.0f) {
        ma_engine_set_volume(m_impl->engine.get(), linearGain);
    }
}

AudioVoiceHandle AudioManager::CreateVoice(const AudioSourceDesc& desc) {
    if (!IsReady() || !m_fileSystem || !IsValidDesc(desc)) return {};
    const std::string normalized =
        m_fileSystem->NormalizeAssetPath(desc.assetPath);
    if (normalized.empty()) return {};

    const std::filesystem::path path =
        m_fileSystem->GetAssetsFolder() / std::filesystem::u8path(normalized);
    ma_uint32 flags = desc.loadMode == AudioLoadMode::Decode
        ? MA_SOUND_FLAG_DECODE : MA_SOUND_FLAG_STREAM;
    if (!desc.spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

    auto pending = std::make_unique<ma_sound>();
#ifdef _WIN32
    const ma_result result = ma_sound_init_from_file_w(
        m_impl->engine.get(), path.wstring().c_str(), flags,
        nullptr, nullptr, pending.get());
#else
    const ma_result result = ma_sound_init_from_file(
        m_impl->engine.get(), path.string().c_str(), flags,
        nullptr, nullptr, pending.get());
#endif
    if (result != MA_SUCCESS) return {};
    MaSoundPtr sound(pending.release());
    ma_sound_set_looping(sound.get(), desc.looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), desc.volume);
    ma_sound_set_pitch(sound.get(), desc.pitch);
    if (desc.spatial) {
        ma_sound_set_min_distance(sound.get(), desc.minDistance);
        ma_sound_set_max_distance(sound.get(), desc.maxDistance);
    }

    uint32_t index;
    if (!m_impl->freeSlots.empty()) {
        index = m_impl->freeSlots.back();
        m_impl->freeSlots.pop_back();
    } else {
        if (m_impl->voices.size() >= InvalidAudioId) return {};
        index = static_cast<uint32_t>(m_impl->voices.size());
        m_impl->voices.emplace_back();
    }
    auto& slot = m_impl->voices[index];
    slot.generation = m_nextGeneration++;
    if (m_nextGeneration == 0) ++m_nextGeneration;
    slot.sound = std::move(sound);
    slot.state = AudioPlaybackState::Stopped;
    slot.looping = desc.looping;
    slot.oneShot = false;
    slot.spatial = desc.spatial;
    return {index, slot.generation};
}

void AudioManager::DestroyVoice(AudioVoiceHandle handle) {
    if (!IsValidHandle(handle)) return;
    m_impl->Release(handle.index);
}

bool AudioManager::Play(AudioVoiceHandle handle) {
    auto* slot = m_impl->Find(handle);
    if (!slot) return false;
    // miniaudio rewinds automatically when starting a sound at its end.
    if (ma_sound_start(slot->sound.get()) != MA_SUCCESS) return false;
    slot->state = AudioPlaybackState::Playing;
    return true;
}

bool AudioManager::Pause(AudioVoiceHandle handle) {
    auto* slot = m_impl->Find(handle);
    if (!slot || slot->state != AudioPlaybackState::Playing) return false;
    if (ma_sound_stop(slot->sound.get()) != MA_SUCCESS) return false;
    slot->state = AudioPlaybackState::Paused;
    return true;
}

bool AudioManager::Stop(AudioVoiceHandle handle) {
    auto* slot = m_impl->Find(handle);
    if (!slot) return false;
    if (ma_sound_stop(slot->sound.get()) != MA_SUCCESS) return false;
    slot->state = AudioPlaybackState::Stopped;
    return ma_sound_seek_to_pcm_frame(slot->sound.get(), 0) == MA_SUCCESS;
}

AudioPlaybackState AudioManager::GetPlaybackState(
    AudioVoiceHandle handle) const {
    const auto* slot = m_impl->Find(handle);
    if (!slot) return AudioPlaybackState::Stopped;
    if (!slot->looping && slot->state == AudioPlaybackState::Playing &&
        ma_sound_at_end(slot->sound.get())) {
        return AudioPlaybackState::Stopped;
    }
    return slot->state;
}

bool AudioManager::SetTransform(AudioVoiceHandle handle,
    const glm::vec3& position, const glm::vec3& velocity) {
    auto* slot = m_impl->Find(handle);
    if (!slot || !IsFinite(position) || !IsFinite(velocity)) return false;
    if (slot->spatial) {
        ma_sound_set_position(slot->sound.get(),
            position.x, position.y, position.z);
        ma_sound_set_velocity(slot->sound.get(),
            velocity.x, velocity.y, velocity.z);
    }
    return true;
}

bool AudioManager::SetVoiceVolume(
    AudioVoiceHandle handle, float volume) {
    auto* slot = m_impl->Find(handle);
    if (!slot || !std::isfinite(volume) || volume < 0.f) return false;
    ma_sound_set_volume(slot->sound.get(), volume);
    return true;
}

bool AudioManager::SetListener(const AudioListenerState& state) {
    if (!IsReady() || !IsFinite(state.position) ||
        !IsFinite(state.forward) || !IsFinite(state.up) ||
        !IsFinite(state.velocity)) {
        return false;
    }
    const float forwardLength = glm::length(state.forward);
    const float upLength = glm::length(state.up);
    if (!std::isfinite(forwardLength) || !std::isfinite(upLength) ||
        forwardLength < 0.0001f || upLength < 0.0001f) {
        return false;
    }
    const glm::vec3 forward = state.forward / forwardLength;
    const glm::vec3 up = state.up / upLength;
    if (glm::length(glm::cross(forward, up)) < 0.0001f) return false;

    auto* engine = m_impl->engine.get();
    ma_engine_listener_set_position(engine, 0,
        state.position.x, state.position.y, state.position.z);
    ma_engine_listener_set_direction(engine, 0,
        forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(engine, 0, up.x, up.y, up.z);
    ma_engine_listener_set_velocity(engine, 0,
        state.velocity.x, state.velocity.y, state.velocity.z);
    ma_engine_listener_set_enabled(engine, 0, MA_TRUE);
    return true;
}

void AudioManager::ClearListener() {
    if (IsReady()) {
        ma_engine_listener_set_enabled(m_impl->engine.get(), 0, MA_FALSE);
    }
}

AudioVoiceHandle AudioManager::PlayOneShot(
    const std::string& assetPath,
    const std::optional<glm::vec3>& worldPosition) {
    AudioSourceDesc desc;
    desc.assetPath = assetPath;
    desc.spatial = worldPosition.has_value();
    auto handle = CreateVoice(desc);
    if (!IsValidHandle(handle)) return {};
    if (worldPosition && !SetTransform(handle, *worldPosition)) {
        DestroyVoice(handle);
        return {};
    }
    m_impl->voices[handle.index].oneShot = true;
    if (!Play(handle)) {
        DestroyVoice(handle);
        return {};
    }
    return handle;
}

void AudioManager::Update() {
    if (!IsReady()) return;
    for (uint32_t i = 0; i < m_impl->voices.size(); ++i) {
        auto& slot = m_impl->voices[i];
        if (!slot.sound || slot.looping) continue;
        if (ma_sound_at_end(slot.sound.get())) {
            if (slot.oneShot) m_impl->Release(i);
            else slot.state = AudioPlaybackState::Stopped;
        }
    }
}

bool AudioManager::IsValidHandle(AudioVoiceHandle handle) const {
    return m_impl && m_impl->Find(handle) != nullptr;
}

} // namespace eng
