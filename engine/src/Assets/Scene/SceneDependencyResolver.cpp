#include "Assets/Scene/SceneDependencyResolver.h"
#include "Assets/AssetManager.h"
#include "IO/FileSystem.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Renderer/RenderDevice.h"

#include <string>

namespace eng
{
    
SceneDependencyResolver::SceneDependencyResolver(
    AssetManager& assets,
    RenderDevice& renderDevice,
    const FileSystem& fileSystem,
    const SceneTypeRegistry& registry)
    : m_assetManager(assets), m_renderDevice(renderDevice),
    m_fileSystem(fileSystem), m_registry(registry) {}

SceneLoadResult<ResolvedSceneAssets> 
    SceneDependencyResolver::Resolve(const SceneDesc& desc) const {
    ResolvedSceneAssets assets;
    std::vector<SceneLoadError> errors;
    bool success = true;
    for (const auto& object : desc.objects) {
        success &= ResolveObject(desc.sourcePath, object, assets, errors);
    }

    if (!success) {
        return SceneLoadResult<ResolvedSceneAssets>::Failure(std::move(errors));
    }
    return SceneLoadResult<ResolvedSceneAssets>::Success(std::move(assets));
}

bool SceneDependencyResolver::ResolveObject(
    const std::string& sceneSrc,
    const SceneObjectDesc& object, 
    ResolvedSceneAssets& output, 
    std::vector<SceneLoadError>& errors) const {
    bool success = true;
    SceneDependencyContext objContext{
        .fileSystem = m_fileSystem,
        .assets = m_assetManager, 
        .renderDevice = m_renderDevice,
        .sourcePath = sceneSrc,
        .jsonPath = object.jsonPath,
        .output = output, 
        .errors = errors
    };
    if (object.className.has_value()) {
        const std::string& className = object.className.value();
        const auto* codec = m_registry.FindGameObject(className);
        if (codec == nullptr) {
            success = false;
            objContext.ReportError(SceneLoadErrorCode::UnknownType, 
                "unknown type: " + className);
        } else if (!object.classPayload) {
            success = false;
            objContext.ReportError(SceneLoadErrorCode::Construction,
                "game object type has no parsed payload: " + className);
        } else if (codec->resolve) {
            success &= codec->resolve(*object.classPayload, objContext);
        }
    } 

    if (object.instance.has_value()) {
        const SceneInstanceDesc& desc = object.instance.value();
        if (desc.kind != SceneInstanceDesc::Kind::Model) {
            success = false;
            objContext.ReportError(SceneLoadErrorCode::Schema,
                "instance kind must be Model");
        } else {
            const std::string normalized =
                m_fileSystem.NormalizeAssetPath(desc.assetPath);
            if (normalized.empty()) {
                success = false;
                objContext.ReportError(SceneLoadErrorCode::AssetNotFound,
                    "model asset was not found or is outside the assets root",
                    desc.assetPath);
            } else {
                std::shared_ptr<Model> model =
                    objContext.assets.LoadModel(normalized);
                if (!model) {
                    success = false;
                    objContext.ReportError(SceneLoadErrorCode::AssetNotFound,
                        "failed to load model", normalized);
                } else {
                    objContext.output.Store(
                        SceneAssetKey{object.jsonPath, "model"}, model);
                }
            }
        }
    }

    for (const auto& comp : object.components) {
        const auto* codec = m_registry.FindComponent(comp.typeName);
        if (codec == nullptr) {
            success = false;
            errors.push_back({
                sceneSrc, comp.jsonPath, 
                SceneLoadErrorCode::UnknownType,
                "unknown type: " + comp.typeName
            });
        } else {
            SceneDependencyContext context{
                .fileSystem = m_fileSystem,
                .assets = m_assetManager, 
                .renderDevice = m_renderDevice,
                .sourcePath = sceneSrc,
                .jsonPath = comp.jsonPath,
                .output = output, 
                .errors = errors
            };
            if (!comp.payload) {
                success = false;
                context.ReportError(SceneLoadErrorCode::Construction,
                    "component has no parsed payload: " + comp.typeName);
            } else if (codec->resolve) {
                success &= codec->resolve(*comp.payload, context);
            }
        }
    }

    for (const auto& child : object.children) {
        success &= ResolveObject(sceneSrc, child, output, errors);
    }

    return success;
}


} // namespace eng
