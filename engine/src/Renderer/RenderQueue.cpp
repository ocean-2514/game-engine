#include "Renderer/RenderQueue.h"

#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/ShaderProgram.h"

#include <algorithm>
#include <iostream>
#include <utility>

namespace eng {

RenderCommand::RenderCommand(
    std::shared_ptr<Mesh> meshValue,
    std::shared_ptr<Material> materialValue,
    const glm::mat4& modelMatrix)
    : mesh(std::move(meshValue)),
      material(std::move(materialValue)),
      modelMatrix(modelMatrix) {}

bool RenderQueue::IsValid(const RenderCommand& command) {
    return command.mesh && command.mesh->IsValid() &&
           command.material && command.material->IsValid();
}

void RenderQueue::Submit(RenderCommand command) {
    if (!IsValid(command)) {
        std::cout << "RenderQueue::Submit: command requires a valid Mesh and Material\n";
        return;
    }

    const uint64_t revision = command.material->GetRevision();
    m_commands.push_back({std::move(command), revision});
}

RenderQueue::ShaderState& RenderQueue::GetShaderState(
    const std::shared_ptr<ShaderProgram>& shader) {
    for (auto& state : m_shaderStates) {
        if (state.shader.lock() == shader) {
            return state;
        }
    }

    m_shaderStates.push_back({shader, {}, 0});
    return m_shaderStates.back();
}

void RenderQueue::Execute() {
    m_shaderStates.erase(
        std::remove_if(
            m_shaderStates.begin(),
            m_shaderStates.end(),
            [](const ShaderState& state) { return state.shader.expired(); }),
        m_shaderStates.end());

    for (const auto& queued : m_commands) {
        const auto& command = queued.command;
        if (!IsValid(command)) {
            std::cout << "RenderQueue::Execute: command became invalid after submission\n";
            continue;
        }
        if (command.material->GetRevision() != queued.materialRevision) {
            std::cout << "RenderQueue::Execute: material changed after submission; command skipped\n";
            continue;
        }

        const auto& shader = command.material->GetShaderProgram();
        if (m_currentShader.lock() != shader) {
            shader->Bind();
            m_currentShader = shader;
        }

        // set modelMatrix
        shader->setMat4f("uModel", command.modelMatrix);

        auto& shaderState = GetShaderState(shader);
        const bool parametersAreCurrent =
            shaderState.material.lock() == command.material &&
            shaderState.materialRevision == command.material->GetRevision();
        if (!parametersAreCurrent) {
            command.material->ApplyParameters();
            shaderState.material = command.material;
            shaderState.materialRevision = command.material->GetRevision();
        }

        command.mesh->Bind();
        command.mesh->Draw();
    }

    Clear();
}

void RenderQueue::Clear() {
    m_commands.clear();
}

void RenderQueue::InvalidateStateCache() {
    m_currentShader.reset();
    m_shaderStates.clear();
}

} // namespace eng
