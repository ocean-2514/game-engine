#include "Renderer/RenderQueue.h"

#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Animation.h"
#include "Renderer/Texture.h"
#include "Renderer/MeshFactory.h"
#include "IO/FileSystem.h"
#include "Assets/AssetManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

namespace eng {

namespace {

bool IsValidViewport(const Viewport& viewport) {
    return viewport.x >= 0 && viewport.y >= 0 &&
        viewport.width > 0 && viewport.height > 0;
}

bool FitsFramebuffer(const Viewport& viewport, const Viewport& framebuffer) {
    return IsValidViewport(viewport) &&
        static_cast<int64_t>(viewport.x) + viewport.width <= framebuffer.width &&
        static_cast<int64_t>(viewport.y) + viewport.height <= framebuffer.height;
}

bool IsFinite(const glm::mat4& matrix) {
    const float* values = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(values[i])) return false;
    }
    return true;
}

bool IsFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsFinite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z) && std::isfinite(value.w);
}

float ViewSpaceDepth(
    const glm::mat4& model,
    const glm::mat4& cameraView) {
    const glm::vec4 position =
        cameraView * model * glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
    return position.z;
}

void ApplyCameraData(ShaderProgram* shader, const CameraData& camera) {
    shader->SetVec3("uViewPos", camera.position);
    shader->SetMat4f("uView", camera.view);
    shader->SetMat4f("uProjection", camera.projection);
}

void ApplyLightingData(ShaderProgram* shader, 
    const LightingData& lighting) {
    shader->SetVec3("uAmbientColor", lighting.ambientColor);
    shader->SetFloat("uAmbientIntensity", lighting.ambientIntensity);

    std::size_t directionalLightNum = lighting.directionalLights.size();
    if (directionalLightNum > LightingLimits::MaxDirectionalLights) {
        std::cout << "ApplyLightingData: number of directional lights exceeds "
            "the maximum limit " + std::to_string(LightingLimits::MaxDirectionalLights) + 
            ", and the excess portion will be ignored" << std::endl;
        directionalLightNum = LightingLimits::MaxDirectionalLights;
    }
    shader->SetInt("uDirectionalLightCount", static_cast<int>(directionalLightNum));
    for (std::size_t i = 0; i < directionalLightNum; ++i) {
        const auto& directionalLight = lighting.directionalLights[i];
        const std::string prefix = "uDirectionalLights[" + std::to_string(i) + "].";
        shader->SetVec3(prefix + "direction", directionalLight.direction);
        shader->SetFloat(prefix + "intensity", directionalLight.intensity);
        shader->SetVec3(prefix + "color", directionalLight.color);
    }

    std::size_t pointLightNum = lighting.pointLights.size();
    if (pointLightNum > LightingLimits::MaxPointLights) {
        std::cout << "ApplyLightingData: number of point lights exceeds "
            "the maximum limit " + std::to_string(LightingLimits::MaxPointLights) + 
            ", and the excess portion will be ignored" << std::endl;
        pointLightNum = LightingLimits::MaxPointLights;
    }
    shader->SetInt("uPointLightCount", static_cast<int>(pointLightNum));
    for (std::size_t i = 0; i < pointLightNum; ++i) {
        const auto& pointLight = lighting.pointLights[i];
        const std::string prefix = "uPointLights[" + std::to_string(i) + "].";
        shader->SetVec3(prefix + "position", pointLight.position);
        shader->SetFloat(prefix + "range", pointLight.range);
        shader->SetVec3(prefix + "color", pointLight.color);
        shader->SetFloat(prefix + "intensity", pointLight.intensity);
    }
    
    std::size_t spotLightNum = lighting.spotLights.size();
    if (spotLightNum > LightingLimits::MaxSpotLights) {
        std::cout << "ApplyLightingData: number of spot lights exceeds "
            "the maximum limit " + std::to_string(LightingLimits::MaxSpotLights) + 
            ", and the excess portion will be ignored" << std::endl;
        spotLightNum = LightingLimits::MaxSpotLights;
    }
    shader->SetInt("uSpotLightCount", static_cast<int>(spotLightNum));
    for (std::size_t i = 0; i < spotLightNum; ++i) {
        const auto& spotLight = lighting.spotLights[i];
        const std::string prefix = "uSpotLights[" + std::to_string(i) + "].";
        shader->SetVec3(prefix + "position", spotLight.position);
        shader->SetFloat(prefix + "range", spotLight.range);
        shader->SetVec3(prefix + "direction", spotLight.direction);
        shader->SetFloat(prefix + "intensity", spotLight.intensity);
        shader->SetVec3(prefix + "color", spotLight.color);
        shader->SetFloat(prefix + "innerConeCos", spotLight.innerConeCos);
        shader->SetFloat(prefix + "outerConeCos", spotLight.outerConeCos);
    }
}


}

RenderCommand::RenderCommand(
    std::shared_ptr<Mesh> meshValue,
    std::shared_ptr<Material> materialValue,
    const glm::mat4& modelMatrix)
    : mesh(std::move(meshValue)),
      material(std::move(materialValue)),
      modelMatrix(modelMatrix) {}

bool RenderQueue::Init(RenderDevice& device, 
    FileSystem& fileSystem, AssetManager& assetManager) {
    m_quadMesh = MeshFactory::CreatePlane(device, 1.0f, 1.0f);
    const auto vsCode = fileSystem.LoadAssetTextFile("shader/default/screen_quad_vs.glsl");
    const auto fsCode = fileSystem.LoadAssetTextFile("shader/default/screen_quad_fs.glsl");
    m_defaultScreenShader = device.CreateShaderProgram(vsCode, fsCode);

    m_screenRenderState.depth.depthTestEnable = false;
    m_screenRenderState.depth.depthWriteEnable = false;
    m_screenRenderState.blend.blendEnable = true;
    m_screenRenderState.rasterizer.cullMode = CullMode::None;

    m_defaultTexture = assetManager.LoadTexture(
        {.path = "texture/default/white.png"});

    return m_defaultTexture != nullptr && m_defaultTexture->IsValid()
        && m_quadMesh != nullptr && m_quadMesh->IsValid()
        && m_defaultScreenShader != nullptr 
        && m_defaultScreenShader->IsValid();
}

bool RenderQueue::IsValid(const RenderCommand& command) {
    return command.mesh && command.mesh->IsValid() &&
           command.material && command.material->IsValid();
}

bool RenderQueue::IsValid(const ScreenQuadCommand& command) {
    return (!command.texture || command.texture->IsValid()) &&
        (!command.shader || command.shader->IsValid()) &&
        IsFinite(command.modelMatrix) && IsFinite(command.lowerLeftUV) &&
        IsFinite(command.upperRightUV) && IsFinite(command.color);
}

bool RenderQueue::BeginView(const RenderViewData& viewData) {
    if (m_currentRenderView != nullptr || m_currentScreenQuadView != nullptr) {
        std::cout << "RenderQueue::BeginView: a render view is already open\n";
        return false;
    }
    if (viewData.viewport && !IsValidViewport(*viewData.viewport)) {
        std::cout << "RenderQueue::BeginView: invalid viewport\n";
        return false;
    }

    auto view = std::make_unique<RenderView>(RenderView{viewData});
    m_currentRenderView = view.get();
    m_renderViews.push_back(std::move(view));
    return true;
}

void RenderQueue::Submit(RenderCommand command) {
    if (!IsValid(command)) {
        std::cout << "RenderQueue::Submit: command requires a valid Mesh and Material\n";
        return;
    }
    if (m_currentRenderView == nullptr) {
        std::cout << "RenderQueue::Submit: render view not set\n";
        return;
    }

    const uint64_t revision = command.material->GetRevision();
    m_currentRenderView->commands.push_back({std::move(command), revision});
}

void RenderQueue::EndView() {
    m_currentRenderView = nullptr;
}

bool RenderQueue::BeginScreenView(const ScreenViewDesc& desc) {
    if (m_currentScreenQuadView != nullptr || m_currentRenderView != nullptr) {
        std::cout << "RenderQueue::BeginScreenView: "
            "a screen quad view is already open\n";
        return false;
    }

    if (!std::isfinite(desc.logicalWidth) || desc.logicalWidth <= 0.0f ||
        !std::isfinite(desc.logicalHeight) || desc.logicalHeight <= 0.0f ||
        !IsValidViewport(desc.viewport) ||
        (desc.origin != ScreenOrigin::TopLeft &&
         desc.origin != ScreenOrigin::BottomLeft)) {
        std::cout << "RenderQueue::BeginScreenView: invalid screen view\n";
        return false;
    }

    auto view = std::make_unique<ScreenQuadView>(ScreenQuadView{desc});
    m_currentScreenQuadView = view.get();
    m_screenQuadViews.push_back(std::move(view));
    return true;
}

void RenderQueue::SubmitScreenQuad(ScreenQuadCommand command) {
    if (!IsValid(command)) {
        std::cout << "RenderQueue::SubmitScreenQuad: invalid texture, shader, or draw data\n";
        return;
    }
    if (m_currentScreenQuadView == nullptr) {
        std::cout << "RenderQueue::Submit: screen quad view not set\n";
        return;
    }

    m_currentScreenQuadView->commands.push_back(std::move(command));
}

void RenderQueue::EndScreenView() {
    m_currentScreenQuadView = nullptr;
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

void RenderQueue::Execute(RenderDevice& device,
    const Viewport& framebufferViewport) {
    if (!IsValidViewport(framebufferViewport)) {
        Clear();
        return;
    }
    // World views are always executed before screen views in this version.
    ExecuteRender(device, framebufferViewport);
    ExecuteScreenQuad(device, framebufferViewport);

    Clear();
}

void RenderQueue::ExecuteRender(RenderDevice& device,
    const Viewport& framebufferViewport) {
    m_shaderStates.erase(
        std::remove_if(
            m_shaderStates.begin(),
            m_shaderStates.end(),
            [](const ShaderState& state) { return state.shader.expired(); }),
        m_shaderStates.end());

    for (const auto& view : m_renderViews) {
        const Viewport viewport = view->data.viewport.value_or(framebufferViewport);
        if (!FitsFramebuffer(viewport, framebufferViewport)) {
            std::cout << "RenderQueue::ExecuteRender: invalid viewport\n";
            continue;
        }
        device.SetViewport(viewport);
        const auto& camera = view->data.camera;
        const auto& lighting = view->data.lighting;

        std::stable_sort(view->commands.begin(), view->commands.end(),
            [&camera](const QueuedRenderCommand& queued1, const QueuedRenderCommand& queued2) {
                const RenderCommand& cmd1 = queued1.command;
                const RenderCommand& cmd2 = queued2.command;
                if (cmd1.phase != cmd2.phase)
                    return static_cast<uint8_t>(cmd1.phase) <
                        static_cast<uint8_t>(cmd2.phase);

                if (cmd1.renderOrder != cmd2.renderOrder)
                    return cmd1.renderOrder < cmd2.renderOrder;

                if (cmd1.phase == RenderPhase::Transparent) {
                    // The camera looks down -Z in view space. More negative
                    // values are farther away and must be rendered first.
                    return ViewSpaceDepth(cmd1.modelMatrix, camera.view) <
                        ViewSpaceDepth(cmd2.modelMatrix, camera.view);
                }
                return false;
            }
        );

        std::vector<ShaderProgram*> viewDataAppliedShaders;

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

            const bool viewDataIsCurrent =
                std::find(viewDataAppliedShaders.begin(),
                    viewDataAppliedShaders.end(), shader.get()) !=
                viewDataAppliedShaders.end();
            if (!viewDataIsCurrent) {
                ApplyCameraData(shader.get(), camera);
                ApplyLightingData(shader.get(), lighting);
                viewDataAppliedShaders.push_back(shader.get());
            }

            // set modelMatrix
            shader->SetMat4f("uModel", command.modelMatrix);
            const glm::mat3 normalMatrix = glm::transpose(
                glm::inverse(glm::mat3(command.modelMatrix)));
            shader->SetMat3f("uNormalMatrix", normalMatrix);

            const bool skinned = command.skeletonPose &&
                !command.skeletonPose->skinMatrices.empty();
            shader->SetInt("uSkinned", skinned ? 1 : 0);
            if (skinned) {
                if (command.skeletonPose->skinMatrices.size() >
                    AnimationLimits::MaxBones) {
                    std::cout << "RenderQueue::Execute: skeleton exceeds shader bone limit\n";
                    continue;
                }
                for (std::size_t i = 0;
                     i < command.skeletonPose->skinMatrices.size(); ++i) {
                    shader->SetMat4f(
                        "uBones[" + std::to_string(i) + "]",
                        command.skeletonPose->skinMatrices[i]);
                }
            }

            ApplyRenderState(command.material->GetRenderState(), device);

            command.mesh->Bind();
            command.mesh->Draw();
        }
    }
}

void RenderQueue::ExecuteScreenQuad(RenderDevice& device,
    const Viewport& framebufferViewport) {
    if (m_screenQuadViews.empty()) return;
    if (!m_quadMesh || !m_quadMesh->IsValid() ||
        !m_defaultScreenShader || !m_defaultScreenShader->IsValid()) {
        InvalidateStateCache();
        return;
    }

    for (const auto& view : m_screenQuadViews) {
        const auto& desc = view->desc;
        if (!FitsFramebuffer(desc.viewport, framebufferViewport)) {
            std::cout << "RenderQueue::ExecuteScreenQuad: invalid viewport\n";
            continue;
        }
        ApplyRenderState(m_screenRenderState, device);
        glm::mat4 projection = desc.origin == ScreenOrigin::TopLeft ?
            glm::ortho(0.0f, desc.logicalWidth, desc.logicalHeight, 0.0f) :
            glm::ortho(0.0f, desc.logicalWidth, 0.0f, desc.logicalHeight);

        device.SetViewport(desc.viewport);

        auto& commands = view->commands;
        std::stable_sort(commands.begin(), commands.end(), 
            [](const ScreenQuadCommand& cmd1, const ScreenQuadCommand& cmd2) {
                return cmd1.renderOrder < cmd2.renderOrder;
            });

        for (const auto& cmd : commands) {
            if (!IsValid(cmd)) continue;
            std::shared_ptr<ShaderProgram> shader = 
                !cmd.shader ? m_defaultScreenShader : cmd.shader;
            
            if (m_currentShader.lock() != shader) {
                m_currentShader = shader;
                shader->Bind();
            }
            m_currentTextureMaterial.reset();

            shader->SetMat4f("uModel", cmd.modelMatrix);
            shader->SetVec4("uColor", cmd.color);
            shader->SetMat4f("uProjection", projection);
            shader->SetVec2("uLowerLeftUV", cmd.lowerLeftUV);
            shader->SetVec2("uUpperRightUV", cmd.upperRightUV);
            
            shader->SetTexture("uTexture", 
                cmd.texture ? *cmd.texture : *m_defaultTexture, 0);
            
            m_quadMesh->Bind();
            m_quadMesh->Draw();
        }
    }
    // Screen shaders and textures may overwrite uniforms/bindings cached by
    // the world pass, including when a custom shader is shared there.
    InvalidateStateCache();
}

void RenderQueue::ApplyRenderState(const RenderState& state,
    RenderDevice& device) {
    if (!m_renderStateCacheValid) {
        device.SetRenderState(state);
        m_currentRenderState = state;
        m_renderStateCacheValid = true;
        return;
    }
    if (state.depth != m_currentRenderState.depth) {
        device.SetDepthState(state.depth);
        m_currentRenderState.depth = state.depth;
    }
    if (state.blend != m_currentRenderState.blend) {
        device.SetBlendState(state.blend);
        m_currentRenderState.blend = state.blend;
    }
    if (state.rasterizer != m_currentRenderState.rasterizer) {
        device.SetRasterizerState(state.rasterizer);
        m_currentRenderState.rasterizer = state.rasterizer;
    }
}

void RenderQueue::Clear() {
    m_currentRenderView = nullptr;
    m_currentScreenQuadView = nullptr;
    m_renderViews.clear();
    m_screenQuadViews.clear();
}

void RenderQueue::ShutDown() {
    Clear();
    InvalidateStateCache();
    m_quadMesh.reset();
    m_defaultScreenShader.reset();
    m_defaultTexture.reset();
}

void RenderQueue::InvalidateStateCache() {
    m_currentShader.reset();
    m_currentTextureMaterial.reset();
    m_currentTextureMaterialRevision = 0;
    m_shaderStates.clear();
    m_renderStateCacheValid = false;
}

} // namespace eng
