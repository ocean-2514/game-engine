#include "Scene/Scene.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/AnimationComponent.h"
#include "Scene/Components/SkinnedMeshComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/RigidBodyComponent.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/Model.h"

#include <utility>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace eng {

Scene::Scene() {
    m_physicsWorld = std::make_unique<PhysicsWorld>(
        PhysicsWorldDesc{}
    );
}

void Scene::Update(float deltaTime) {
    if (m_isUpdating || !std::isfinite(deltaTime) || deltaTime < 0.0f) {
        return;
    }
    m_isUpdating = true;

    {
        struct UpdateGuard {
            bool& isUpdating;
            ~UpdateGuard() { isUpdating = false; }
        } guard{m_isUpdating};

        if (m_physicsWorld) {
            const auto& desc = m_physicsWorld->GetDesc();
            float maxAccumulatedTime = desc.maxAccumulatedTime;
            float fixedTimeStep = desc.fixedTimeStep;
            uint32_t maxSubStepFrame = desc.maxSubStepsPerFrame;
            if (deltaTime > 0.0f) {
                m_physicsAccumulator = std::min(
                    m_physicsAccumulator + deltaTime,
                    maxAccumulatedTime);
            }
            uint32_t step = 0;
            while (m_physicsAccumulator >= fixedTimeStep &&
                step < maxSubStepFrame) {
                UpdateObjects(fixedTimeStep, true);
                PushKinematicTransforms();
                m_physicsWorld->Simulate(fixedTimeStep);
                PullDynamicTransforms();
                ++step;
                m_physicsAccumulator -= fixedTimeStep;
            }
            if (step == maxSubStepFrame &&
                m_physicsAccumulator >= fixedTimeStep) {
                // Drop overdue whole steps instead of carrying an unbounded
                // catch-up backlog into later frames.
                m_physicsAccumulator = std::fmod(
                    m_physicsAccumulator, fixedTimeStep);
            }
        }

        UpdateObjects(deltaTime, false);
    }

    // flush commands after objects marked for destroy are destroyed
    FlushPendingCommands();
}

void Scene::Render(RenderQueue& queue, float aspect) {
    Render(queue, m_mainCamera, aspect);
}

void Scene::Render(RenderQueue& queue, CameraComponent* camera, float aspect) {
    if (!IsValidComponent(camera)) return;

    CameraData cameraData{
        camera->GetPosition(),
        camera->GetViewMatrix(),
        camera->GetProjectionMatrix(aspect)
    };
    LightingData lightingData{};
    CollectLightingData(lightingData);
    if (!queue.BeginView({
        cameraData, lightingData
    })) {
        return;
    }

    for (auto& object : m_objects) {
        if (!object->IsAlive()) continue;
        object->RenderTree(queue);
    }

    queue.EndView();
}


void Scene::Clear() {
    if (m_isUpdating) {
        m_pendingCommands.push_back(ClearCommand{});
        return;
    }
    ClearImmediate();
}

GameObject* Scene::CreateObject(std::string name, GameObject* parent) {
    std::unique_ptr<GameObject> object(new GameObject());
    GameObject* result = object.get();
    return AttachObject(std::move(object), std::move(name), parent)
        ? result
        : nullptr;
}

void Scene::UpdateObjects(float deltaTime, bool fixedDeltaTime) {
    if (!m_isUpdating) return;

    for (auto it = m_objects.begin(); it != m_objects.end();) {
        GameObject& object = **it;
        if (object.IsAlive()) {
            if (fixedDeltaTime) {
                object.FixedUpdateTree(deltaTime);
            } else {
                object.UpdateTree(deltaTime);
            }
        }

        if (object.IsAlive()) {
            ++it;
        } else {
            it = m_objects.erase(it);
        }
    }
}

void Scene::PushKinematicTransforms() {
    if (!m_physicsWorld) return;
    for (auto& object : m_objects) {
        PushKinematicTransformsTree(object.get());
    }
}

void Scene::PushKinematicTransformsTree(GameObject* object) {
    if (!object || !object->IsAlive()) return;
    if (auto* rigidBodyComp = object->GetComponent<RigidBodyComponent>()) {
        rigidBodyComp->PushKinematicTransform();
    }

    for (auto& child : object->m_children) {
        PushKinematicTransformsTree(child.get());
    }
}

void Scene::PullDynamicTransforms() {
    if (!m_physicsWorld) return;
    for (auto& object : m_objects) {
        PullDynamicTransformsTree(object.get());
    }
}

void Scene::PullDynamicTransformsTree(GameObject* object) {
    if (!object || !object->IsAlive()) return;
    if (auto* rigidBodyComp = object->GetComponent<RigidBodyComponent>()) {
        rigidBodyComp->PullDynamicTransform();
    }

    for (auto& child : object->m_children) {
        PullDynamicTransformsTree(child.get());
    }
}

bool Scene::AttachObject(
    std::unique_ptr<GameObject> object,
    std::string name,
    GameObject* parent) {
    if (m_isUpdating) {
        if (!object ||
            (parent != nullptr &&
                (!IsKnownObject(parent) || !parent->IsAlive()))) {
            return false;
        }
        m_pendingCommands.push_back(
            AttachObjectCommand{std::move(object), std::move(name), parent}
        );
        return true;
    }

    return AttachObjectImmediate(std::move(object), std::move(name), parent);
}

bool Scene::SetParent(GameObject* object, GameObject* parent) {
    if (m_isUpdating) {
        if (!IsKnownObject(object) || !object->IsAlive() ||
            (parent != nullptr &&
                (!IsKnownObject(parent) || !parent->IsAlive())) ||
            object == parent) {
            return false;
        }
        m_pendingCommands.push_back(
            ReparentCommand{object, parent}
        );
        return true;
    }

    return SetParentImmediate(object, parent);
}

bool Scene::Contains(const GameObject* object) const {
    return object != nullptr && ContainsIn(m_objects, object);
}

bool Scene::ContainsIn(
    const ObjectContainer& objects,
    const GameObject* object) {
    for (const auto& candidate : objects) {
        if (candidate.get() == object || ContainsIn(candidate->m_children, object)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<GameObject> Scene::ExtractFrom(
    ObjectContainer& objects,
    GameObject* object) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->get() == object) {
            auto result = std::move(*it);
            objects.erase(it);
            return result;
        }

        auto result = ExtractFrom((*it)->m_children, object);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

bool Scene::WouldCreateCycle(
    const GameObject* object,
    const GameObject* newParent) {
    for (auto current = newParent; current != nullptr; current = current->m_parent) {
        if (current == object) {
            return true;
        }
    }
    return false;
}

bool Scene::IsStagedObject(const GameObject* object) const {
    if (object == nullptr) {
        return false;
    }

    for (const auto& command : m_pendingCommands) {
        const auto* attach = std::get_if<AttachObjectCommand>(&command);
        if (attach != nullptr && attach->object.get() == object) {
            return true;
        }
    }
    return false;
}

bool Scene::IsKnownObject(const GameObject* object) const {
    return Contains(object) || IsStagedObject(object);
}

bool Scene::IsValidComponent(const Component* component) const {
    return component != nullptr && component->IsAlive() &&
        IsKnownObject(component->GetOwner()) && component->GetOwner()->IsAlive();
}


void Scene::ClearImmediate() {
    m_mainCamera = nullptr;
    m_objects.clear();
    m_physicsAccumulator = 0.0f;
}

bool Scene::SetParentImmediate(GameObject* object, GameObject* parent) {
    if (object == nullptr || !Contains(object) ||
        (parent != nullptr && (!Contains(parent) || !parent->IsAlive())) ||
        WouldCreateCycle(object, parent) || !object->IsAlive()) {
        return false;
    }
    if (object->m_parent == parent) {
        return true;
    }

    auto ownedObject = ExtractFrom(m_objects, object);
    if (!ownedObject) {
        return false;
    }

    object->m_parent = parent;
    if (parent != nullptr) {
        parent->m_children.push_back(std::move(ownedObject));
    } else {
        m_objects.push_back(std::move(ownedObject));
    }
    return true;
}

bool Scene::AttachObjectImmediate(std::unique_ptr<GameObject> object, 
    std::string name, GameObject* parent) {

    if (!object || (parent != nullptr && (!Contains(parent) || !parent->IsAlive())) || 
        !object->IsAlive()) {
        return false;
    }
    object->SetName(std::move(name));
    object->m_parent = parent;
    GameObject* attachedObject = object.get();
    if (parent != nullptr) {
        parent->m_children.push_back(std::move(object));
    } else {
        m_objects.push_back(std::move(object));
    }
    attachedObject->AttachToScene(*this);
    return true;
}

void Scene::FlushPendingCommands() {
    for (auto& cmd : m_pendingCommands) {
        std::visit([this](auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, AttachObjectCommand>) {
                AttachObjectImmediate(std::move(value.object), std::move(value.name), value.parent);
            } else if constexpr (std::is_same_v<T, ReparentCommand>) {
                SetParentImmediate(value.object, value.parent);
            } else if constexpr (std::is_same_v<T, ClearCommand>) {
                ClearImmediate();
            }
        }, cmd);
    }
    m_pendingCommands.clear();
}

GameObject* Scene::InstantiateModel(
    const std::shared_ptr<Model>& model,
    GameObject* parent) {
    if (!model || !model->IsValid()) {
        std::cout << "Scene::InstantiateModel: model is null or invalid\n";
        return nullptr;
    }
    if (parent != nullptr &&
        (!IsKnownObject(parent) || !parent->IsAlive())) {
        std::cout << "Scene::InstantiateModel: parent does not belong to this scene\n";
        return nullptr;
    }

    const auto& rootNode = model->GetNodes()[model->GetRootNodeIndex()];
    const std::string instanceName = !model->GetName().empty()
        ? model->GetName()
        : (!rootNode.name.empty() ? rootNode.name : "Model");
    GameObject* instanceRoot = CreateObject(instanceName, parent);
    if (!instanceRoot) {
        return nullptr;
    }
    std::shared_ptr<SkeletonPose> pose;
    if (!model->GetBones().empty() || !model->GetAnimationClips().empty()) {
        auto* animation = instanceRoot->AddComponent<AnimationComponent>(model);
        if (!animation) {
            instanceRoot->MarkForDestroy();
            return nullptr;
        }
        pose = animation->GetPose();
    }
    if (!ProcessModelNode(
            *model, model->GetRootNodeIndex(), instanceRoot, pose,
            instanceRoot)) {
        // In staged mode the dead root will be discarded when commands flush;
        // in immediate mode it will be removed by the next Scene::Update.
        instanceRoot->MarkForDestroy();
        std::cout << "Scene::InstantiateModel: failed to create model hierarchy\n";
        return nullptr;
    }
    return instanceRoot;
}

GameObject* Scene::ProcessModelNode(
    const Model& model,
    uint32_t nodeIndex,
    GameObject* parent,
    const std::shared_ptr<SkeletonPose>& pose,
    GameObject* modelRoot) {
    const auto& nodes = model.GetNodes();
    const auto& meshes = model.GetMeshes();
    const auto& materials = model.GetMaterials();
    if (nodeIndex >= nodes.size() || parent == nullptr) {
        return nullptr;
    }
    const auto& node = nodes[nodeIndex];
    GameObject* obj = CreateObject(node.name, parent);
    if (!obj) {
        return nullptr;
    }
    obj->SetLocalTransform(node.localTransform);

    for (auto meshIndex : node.meshIndices) {
        if (meshIndex >= meshes.size()) return nullptr;
        const auto& mesh = meshes[meshIndex];
        if (mesh.materialIndex >= materials.size()) return nullptr;

        GameObject* meshObj = CreateObject(mesh.name, obj);
        if (!meshObj) return nullptr;
        Component* component = mesh.skinned
            ? static_cast<Component*>(meshObj->AddComponent<SkinnedMeshComponent>(
                mesh.mesh, materials[mesh.materialIndex], pose, modelRoot))
            : static_cast<Component*>(meshObj->AddComponent<MeshComponent>(
                mesh.mesh, materials[mesh.materialIndex]));
        if (!component) {
            return nullptr;
        }
    }

    for (auto childIndex : node.children) {
        if (childIndex >= nodes.size() ||
            !ProcessModelNode(model, childIndex, obj, pose, modelRoot)) {
            return nullptr;
        }
    }

    return obj;
}

void Scene::CollectLightingData(LightingData& data) const {
    for (const auto& object : m_objects) {
        CollectLightingDataRecursive(object.get(), data);
    }
}

void Scene::CollectLightingDataRecursive(const GameObject* object, 
    LightingData& data) const {
    if (object == nullptr || !object->IsAlive()) {
        return;
    }
    
    if (const auto* directionalLight = 
        object->GetComponent<DirectionalLightComponent>(); 
        directionalLight != nullptr) {
        data.directionalLights.push_back({
            directionalLight->GetDirection(),
            directionalLight->GetIntensity(),
            directionalLight->GetColor()
        });
    }
    if (const auto* pointLight = 
        object->GetComponent<PointLightComponent>();
        pointLight != nullptr) {
        data.pointLights.push_back({
            pointLight->GetPosition(),
            pointLight->GetRange(), 
            pointLight->GetColor(), 
            pointLight->GetIntensity()
        });
    }
    if (const auto* spotLight = 
        object->GetComponent<SpotLightComponent>();
        spotLight != nullptr) {
        data.spotLights.push_back({
            spotLight->GetPosition(),
            spotLight->GetRange(), 
            spotLight->GetDirection(), 
            spotLight->GetIntensity(), 
            spotLight->GetColor(), 
            spotLight->GetInnerConeCos(),
            spotLight->GetOuterConeCos()
        });
    } 

    for (const auto& child : object->m_children) {
        CollectLightingDataRecursive(child.get(), data);
    }
}

CameraComponent* Scene::GetMainCamera() {
    return m_mainCamera;
}

const CameraComponent* Scene::GetMainCamera() const {
    return m_mainCamera;
}

bool Scene::SetMainCamera(CameraComponent* camera) {
    if (camera == nullptr) {
        m_mainCamera = nullptr;
        return true;
    }

    if (!IsValidComponent(camera)) {
        std::cout << "Scene::SetMainCamera: camera does not belong to this scene or is not alive\n";
        return false;
    }

    m_mainCamera = camera;
    return true;
}

bool Scene::SetPhysicsWorld(const PhysicsWorldDesc& desc) {
    if (m_isUpdating || !m_objects.empty() ||
        !m_pendingCommands.empty() || !desc.IsValid()) return false;
    auto world = std::make_unique<PhysicsWorld>(desc);
    if (!world->IsValid()) return false;
    m_physicsWorld = std::move(world);
    m_physicsAccumulator = 0.0f;
    return true;
}

PhysicsWorld* Scene::GetPhysicsWorld() {
    return m_physicsWorld.get();
}

const PhysicsWorld* Scene::GetPhysicsWorld() const {
    return m_physicsWorld.get();
}

std::size_t Scene::GetRootObjectCount() const {
    return m_objects.size();
}

GameObject* Scene::GetRootObject(std::size_t index) {
    return index < m_objects.size() ? m_objects[index].get() : nullptr;
}

const GameObject* Scene::GetRootObject(std::size_t index) const {
    return index < m_objects.size() ? m_objects[index].get() : nullptr;
}

} // namespace eng
