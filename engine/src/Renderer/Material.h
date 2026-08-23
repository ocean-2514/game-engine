#ifndef O_MATERIAL
#define O_MATERIAL

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include <variant>

#include <glm/glm.hpp>

#include "Renderer/RenderState.h"

namespace eng {

class ShaderProgram;
class RenderQueue;
class Texture;

class Material
{
public:
    Material() = default;
    explicit Material(std::shared_ptr<ShaderProgram> shaderProgram);

    void SetShaderProgram(std::shared_ptr<ShaderProgram> shaderProgram);
    SurfaceMode GetSurfaceMode() const;
    void SetSurfaceMode(SurfaceMode mode);
    RenderPhase GetRenderPhase() const;
    void SetRenderPhase(RenderPhase phase);
    int32_t GetDefaultRenderOrder() const;
    void SetDefaultRenderOrder(int32_t order);
    const RenderState& GetRenderState() const;
    void SetRenderState(const RenderState& state);
    void SetDepthState(const DepthState& state);
    void SetBlendState(const BlendState& state);
    void SetRasterizerState(const RasterizerState& state);
    void SetParam(std::string name, int value);
    void SetParam(std::string name, float value);
    void SetParam(std::string name, const glm::vec3& value);
    void SetParam(std::string name, const glm::vec2& value);
    void SetTexture(std::string name, std::shared_ptr<Texture> texture);
    std::shared_ptr<Material> Clone() const;
    void Bind() const;
    bool IsValid() const;

private:
    using ParameterValue = std::variant<int, float, glm::vec3, glm::vec2>;

    void SetParamValue(std::string name, ParameterValue value);
    void ApplyParameters() const;
    bool HasTextures() const;
    void IncrementRevision();
    uint64_t GetRevision() const;
    const std::shared_ptr<ShaderProgram>& GetShaderProgram() const;

    std::shared_ptr<ShaderProgram> m_shaderProgram;
    SurfaceMode m_surfaceMode = SurfaceMode::Opaque;
    RenderPhase m_renderPhase = RenderPhase::Opaque;
    int32_t m_defaultRenderOrder = 0;
    RenderState m_renderState{};
    std::unordered_map<std::string, ParameterValue> m_params;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    uint64_t m_revision = 1;

    friend class RenderQueue;
};


    
} // namespace eng



#endif
