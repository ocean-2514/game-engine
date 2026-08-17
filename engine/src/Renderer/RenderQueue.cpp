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

bool RenderQueue::BeginView(const CameraData& camera) {
    if (m_currentView != nullptr) {
        std::cout << "RenderQueue::BeginView: a render view is already open\n";
        return false;
    }

    auto view = std::make_unique<RenderView>(RenderView{camera});
    m_currentView = view.get();
    m_views.push_back(std::move(view));
    return true;
}

void RenderQueue::Submit(RenderCommand command) {
    if (!IsValid(command)) {
        std::cout << "RenderQueue::Submit: command requires a valid Mesh and Material\n";
        return;
    }
    if (m_currentView == nullptr) {
        std::cout << "RenderQueue::Submit: render view not set\n";
        return;
    }

    const uint64_t revision = command.material->GetRevision();
    m_currentView->commands.push_back({std::move(command), revision});
}

void RenderQueue::EndView() {
    m_currentView = nullptr;
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

    for (const auto& view : m_views) {
        std::vector<ShaderProgram*> cameraDataAppliedShaders;

        for (const auto& queued : view->commands) {
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

            auto& shaderState = GetShaderState(shader);
            const bool texturesAreCurrent =
                !command.material->HasTextures() ||
                (m_currentTextureMaterial.lock() == command.material &&
                 m_currentTextureMaterialRevision ==
                    command.material->GetRevision());
            const bool parametersAreCurrent =
                shaderState.material.lock() == command.material &&
                shaderState.materialRevision == command.material->GetRevision() &&
                texturesAreCurrent;
            if (!parametersAreCurrent) {
                command.material->ApplyParameters();
                shaderState.material = command.material;
                shaderState.materialRevision = command.material->GetRevision();
                if (command.material->HasTextures()) {
                    m_currentTextureMaterial = command.material;
                    m_currentTextureMaterialRevision =
                        command.material->GetRevision();
                }
            }

            const bool cameraDataIsCurrent =
                std::find(cameraDataAppliedShaders.begin(),
                    cameraDataAppliedShaders.end(), shader.get()) !=
                cameraDataAppliedShaders.end();
            if (!cameraDataIsCurrent) {
                shader->SetMat4f("uView", view->camera.view);
                shader->SetMat4f("uProjection", view->camera.projection);
                cameraDataAppliedShaders.push_back(shader.get());
            }

            // set modelMatrix
            shader->SetMat4f("uModel", command.modelMatrix);

            command.mesh->Bind();
            command.mesh->Draw();
        }
    }

    Clear();
}

void RenderQueue::Clear() {
    m_currentView = nullptr;
    m_views.clear();
}

void RenderQueue::InvalidateStateCache() {
    m_currentShader.reset();
    m_currentTextureMaterial.reset();
    m_currentTextureMaterialRevision = 0;
    m_shaderStates.clear();
}

} // namespace eng
