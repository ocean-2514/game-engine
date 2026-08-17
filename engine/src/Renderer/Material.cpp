#include "Renderer/Material.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Texture.h"

#include <iostream>
#include <utility>
#include <type_traits>

namespace eng {

Material::Material(std::shared_ptr<ShaderProgram> shaderProgram)
    : m_shaderProgram(std::move(shaderProgram)) {}
    
void Material::SetShaderProgram(std::shared_ptr<ShaderProgram> shaderProgram) {
    if (m_shaderProgram == shaderProgram) {
        return;
    }
    m_shaderProgram = std::move(shaderProgram);
    IncrementRevision();
}

void Material::SetParam(std::string name, int value) {
    SetParamValue(std::move(name), value);
}

void Material::SetParam(std::string name, float value) {
    SetParamValue(std::move(name), value);
}

void Material::SetParam(std::string name, const glm::vec3& value) {
    SetParamValue(std::move(name), value);
}

void Material::SetParam(std::string name, const glm::vec2& value) {
    SetParamValue(std::move(name), value);
}

void Material::SetParamValue(std::string name, ParameterValue value) {
    const auto it = m_params.find(name);
    if (it != m_params.end() && it->second == value) {
        return;
    }
    m_params.insert_or_assign(std::move(name), std::move(value));
    IncrementRevision();
}

void Material::SetTexture(std::string name, std::shared_ptr<Texture> texture) {
    if (!texture) {
        if (m_textures.erase(name) != 0) {
            IncrementRevision();
        }
        return;
    }

    const auto it = m_textures.find(name);
    if (it != m_textures.end() && it->second.get() == texture.get()) {
        return;
    }
    m_textures.insert_or_assign(std::move(name), std::move(texture));
    IncrementRevision();
}


void Material::Bind() const {
    if (!m_shaderProgram) {
        std::cout << "Material::Bind: ShaderProgram not set" << std::endl;
        return;
    }

    m_shaderProgram->Bind();
    ApplyParameters();
}

void Material::ApplyParameters() const {
    if (!m_shaderProgram) {
        return;
    }
    for (const auto& [name, parameter] : m_params) {
        std::visit([this, &name](const auto& value) {
            using ValueType = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, int>) {
                m_shaderProgram->SetInt(name, value);
            } else if constexpr (std::is_same_v<ValueType, float>) {
                m_shaderProgram->SetFloat(name, value);
            } else if constexpr (std::is_same_v<ValueType, glm::vec3>) {
                m_shaderProgram->SetVec3(name, value);
            } else if constexpr (std::is_same_v<ValueType, glm::vec2>) {
                m_shaderProgram->SetVec2(name, value);
            }
        }, parameter);
    }
    uint32_t textureUnit = 0;
    for (const auto& [name, texture] : m_textures) {
        m_shaderProgram->SetTexture(name, *texture, textureUnit++);
    }
}

bool Material::IsValid() const {
    if (!m_shaderProgram) {
        return false;
    }
    for (const auto& [name, texture] : m_textures) {
        if (!texture || !texture->IsValid()) {
            return false;
        }
    }
    return true;
}

bool Material::HasTextures() const {
    return !m_textures.empty();
}

void Material::IncrementRevision() {
    if (++m_revision == UINT64_MAX) {
        m_revision = 0;
    }
}


uint64_t Material::GetRevision() const {
    return m_revision;
}

const std::shared_ptr<ShaderProgram>& Material::GetShaderProgram() const {
    return m_shaderProgram;
}






} // namespace eng
