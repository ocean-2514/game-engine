#include "Assets/Scene/SceneInstantiator.h"
#include "Renderer/Model.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include <unordered_map>
#include <string>

namespace eng
{

namespace {
    void AppendErrors(
        std::vector<SceneLoadError>& errors,
        const std::vector<SceneLoadError>& append) {
        errors.insert(errors.end(), append.begin(), append.end());
    }

    struct ContextPathGuard {
        SceneLoadContext& context;
        std::string oldPath;

        ContextPathGuard(SceneLoadContext& context, const std::string& path)
            : context(context), oldPath(std::move(context.jsonPath)) {
            context.jsonPath = path;
        }
        ~ContextPathGuard() { context.jsonPath = std::move(oldPath); }
    };
}
    
SceneInstantiator::SceneInstantiator(const SceneTypeRegistry& registry)
    : m_registry(registry) {}

SceneLoadResult<std::unique_ptr<Scene>> 
    SceneInstantiator::CreateScene(
    const SceneDesc& desc,
    const ResolvedSceneAssets& resolved,
    SceneLoadContext& context) const {
    std::vector<SceneLoadError> errors;
    std::unordered_map<std::string, GameObject*> objects;
    Scene::ObjectContainer roots;
    context.sourcePath = desc.sourcePath;
    for (const auto& objDesc : desc.objects) {
        auto object = CreateObject(
            objDesc, resolved, context, errors, objects);
        if (object) roots.push_back(std::move(object));
    }

    if (!errors.empty()) {
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            std::move(errors));
    }

    CameraComponent* mainCamera = nullptr;
    const auto& settings = desc.settings;
    if (settings.mainCameraId.has_value()) {
        const auto& id = settings.mainCameraId.value();
        auto it = objects.find(id);
        if (it == objects.end()) {
            context.jsonPath = "$.settings.mainCamera";
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::InvalidReference,
                "cannot find main camera id: " + id
            ));
        } else {
            mainCamera = it->second->GetComponent<CameraComponent>();
            if (!mainCamera) {
                context.jsonPath = "$.settings.mainCamera";
                errors.push_back(context.MakeError(
                    SceneLoadErrorCode::InvalidReference,
                    "main camera object has no CameraComponent: " + id));
            }
        }
    }

    auto scene = std::make_unique<Scene>();
    context.jsonPath = "$.settings.physics";
    if (!scene->SetPhysicsWorld(settings.physics.world)) {
        errors.push_back(context.MakeError(
                SceneLoadErrorCode::Schema,
                "failed to create physics world"));
    }

    if (!errors.empty()) {
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            std::move(errors));
    }

    scene->SetName(desc.name);
    context.jsonPath = "$.objects";
    if (!scene->CommitObjects(std::move(roots))) {
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "failed to commit detached scene objects"));
    }
    if (mainCamera && !scene->SetMainCamera(mainCamera)) {
        context.jsonPath = "$.settings.mainCamera";
        return SceneLoadResult<std::unique_ptr<Scene>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "failed to register main camera"));
    }
    return SceneLoadResult<std::unique_ptr<Scene>>::Success(std::move(scene));
}

std::unique_ptr<GameObject> SceneInstantiator::CreateObject(
    const SceneObjectDesc& desc,
    const ResolvedSceneAssets& resolved,
    SceneLoadContext& context, 
    std::vector<SceneLoadError>& errors,
    std::unordered_map<std::string, GameObject*>& objects) const {
    ContextPathGuard pathGuard(context, desc.jsonPath);
    std::unique_ptr<GameObject> object;
    if (desc.className.has_value()) {
        const auto& className = desc.className.value();
        const auto* codec = m_registry.FindGameObject(className);
        if (!codec) {
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::UnknownType, 
                "unknown type: " + className
            ));
            return nullptr;
        }
        if (!codec->create || !desc.classPayload) {
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::Construction,
                "game object type is not constructible: " + className));
            return nullptr;
        }
        auto result = codec->create(*desc.classPayload,
            resolved, context);
        if (result.HasError()) {
            AppendErrors(errors, *result.GetErrors());
            return nullptr;
        } 
        object = std::move(result.Value());
    } else if (desc.instance.has_value()) {
        auto model = resolved.Find<Model>(
            SceneAssetKey{desc.jsonPath, "model"});
        if (!model) {
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::Construction,
                "resolved model dependency is missing"));
            return nullptr;
        }
        object = Scene::BuildDetachedModel(model);
        if (!object) {
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::Construction,
                "failed to build model instance hierarchy"));
            return nullptr;
        }
    } else {
        object = std::unique_ptr<GameObject>(new GameObject());
    }
    if (!object) {
        errors.push_back(context.MakeError(
            SceneLoadErrorCode::Construction,
            "game object factory returned null"));
        return nullptr;
    }

    if (!objects.emplace(desc.id, object.get()).second) {
        errors.push_back(context.MakeError(
            SceneLoadErrorCode::Schema,
            "duplicate object id: " + desc.id));
        return nullptr;
    }

    object->SetName(desc.name);
    object->SetLocalTransform(
        desc.transform.position,
        desc.transform.rotation,
        desc.transform.scale
    );

    for (const auto& compDesc : desc.components) {
        auto comp = CreateComponent(compDesc, 
            resolved, context, errors);
        if (!comp) continue;
        if (!object->AttachComponent(std::move(comp))) {
            errors.push_back(context.MakeError(
                SceneLoadErrorCode::Construction,
                "failed to attach component: " + compDesc.typeName));
        }
    }

    for (const auto& childDesc : desc.children) {
        auto child = CreateObject(childDesc, resolved, 
            context, errors, objects);
        if (!child) continue;
        child->m_parent = object.get();
        object->m_children.push_back(std::move(child));
    }
    return object;
}

std::unique_ptr<Component> SceneInstantiator::CreateComponent(
    const SceneComponentDesc& desc,
    const ResolvedSceneAssets& resolved,
    SceneLoadContext& context, 
    std::vector<SceneLoadError>& errors) const {
    ContextPathGuard pathGuard(context, desc.jsonPath);
    const auto* codec = m_registry.FindComponent(desc.typeName);
    if (!codec) {
        errors.push_back(context.MakeError(
            SceneLoadErrorCode::UnknownType,
            "unknown type: " + desc.typeName
        ));
        return nullptr;
    }
    if (!codec->create || !desc.payload) {
        errors.push_back(context.MakeError(
            SceneLoadErrorCode::Construction,
            "component type is not constructible: " + desc.typeName));
        return nullptr;
    }
    auto result = codec->create(*desc.payload, resolved, context);
    if (result.HasError()) {
        AppendErrors(errors, *result.GetErrors());
        return nullptr;
    }
    auto component = std::move(result.Value());
    if (!component) {
        errors.push_back(context.MakeError(
            SceneLoadErrorCode::Construction,
            "component factory returned null: " + desc.typeName));
    }
    return component;
}

} // namespace eng
