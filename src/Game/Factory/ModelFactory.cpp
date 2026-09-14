#include "ModelFactory.h"

std::shared_ptr<eng::Model> ModelFactory::CreatePlayerModel() {
    auto model = std::make_shared<eng::Model>();
    auto& engine = eng::Engine::GetInstance();
    auto& assetManager = engine.GetAssetManager();
    auto& device = engine.GetRenderDevice();
    auto material = assetManager.LoadMaterial("material/base_material.json");
    model->AddMaterial(material);

    auto bodyMesh = eng::MeshFactory::CreateBox(device, 0.7f, 1.0f, 0.7f);
    auto headMesh = eng::MeshFactory::CreateBox(device, 0.5f, 0.5f, 0.5f);
    auto limbMesh = eng::MeshFactory::CreateBox(device, 0.2f, 0.8f, 0.2f);

    model->AddMesh({"body", bodyMesh});
    model->AddMesh({"head", headMesh});
    model->AddMesh({"leftHand", limbMesh});
    model->AddMesh({"rightHand", limbMesh});
    model->AddMesh({"leftLeg", limbMesh});
    model->AddMesh({"rightLeg", limbMesh});

    glm::mat4 mat = glm::mat4{1.0f};

    auto bodyNode = eng::ModelNode{"body"};
    bodyNode.meshIndices.push_back(0);
    bodyNode.children = {1, 2, 3, 4, 5};
    model->AddModelNode(bodyNode);
    auto headNode = eng::ModelNode{"head"};
    headNode.meshIndices.push_back(1);
    headNode.localTransform = 
        glm::translate(mat, glm::vec3(0.0f, 0.6f, 0.0f));
    model->AddModelNode(headNode);
    auto leftHandNode = eng::ModelNode{"leftHand"};
    leftHandNode.meshIndices.push_back(2);
    leftHandNode.localTransform = 
        glm::translate(mat, glm::vec3(-0.45f, 0.0f, 0.0f));
    model->AddModelNode(leftHandNode);
    auto rightHandNode = eng::ModelNode{"rightHand"};
    rightHandNode.meshIndices.push_back(3);
    rightHandNode.localTransform = 
        glm::translate(mat, glm::vec3(0.45f, 0.0f, 0.0f));
    model->AddModelNode(rightHandNode);
    auto leftLegNode = eng::ModelNode{"leftLeg"};
    leftLegNode.meshIndices.push_back(4);
    leftLegNode.localTransform = 
        glm::translate(mat, glm::vec3(-0.2f, -0.7f, 0.0f));
    model->AddModelNode(leftLegNode);
    auto rightLegNode = eng::ModelNode{"rightLeg"};
    rightLegNode.meshIndices.push_back(5);
    rightLegNode.localTransform = 
        glm::translate(mat, glm::vec3(0.2f, -0.7f, 0.0f));
    model->AddModelNode(rightLegNode);

    model->AddAnimationClip(CreateAnimationClip());

    return std::move(model);
}

std::shared_ptr<eng::AnimationClip> 
    ModelFactory::CreateAnimationClip() {
    auto animation = std::make_shared<eng::AnimationClip>();
    animation->duration = 1.5f;
    animation->looping = true;
    animation->name = "Walking";

    auto leftHandTrack = eng::TransformTrack{"leftHand", 2};
    leftHandTrack.positions = {
        {0.0f, glm::vec3(-0.45f, 0.0f, 0.0f)},
        {0.4f, glm::vec3(-0.45f, 0.1f, -0.15f)},
        {0.75f, glm::vec3(-0.45f, 0.0f, 0.0f)},
        {1.1f, glm::vec3(-0.45f, 0.1f, 0.15f)},
        {1.5f, glm::vec3(-0.45f, 0.0f, 0.0f)}
    };
    leftHandTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {0.4f, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {0.75f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {1.1f, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {1.5f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(leftHandTrack);

    auto rightHandTrack = eng::TransformTrack{"rightHand", 3};
    rightHandTrack.positions = {
        {0.0f, glm::vec3(0.45f, 0.0f, 0.0f)},
        {0.4f, glm::vec3(0.45f, -0.1f, 0.15f)},
        {0.75f, glm::vec3(0.45f, 0.0f, 0.0f)},
        {1.1f, glm::vec3(0.45f, -0.1f, -0.15f)},
        {1.5f, glm::vec3(0.45f, 0.0f, 0.0f)}
    };
    rightHandTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {0.4f, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {0.75f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {1.1f, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {1.5f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(rightHandTrack);

    auto leftLegTrack = eng::TransformTrack{"leftLeg", 4};
    leftLegTrack.positions = {
        {0.0f, glm::vec3(-0.2f, -0.7f, 0.0f)},
        {0.4f, glm::vec3(-0.2f, -0.8f, -0.15f)},
        {0.75f, glm::vec3(-0.2f, -0.7f, 0.0f)},
        {1.1f, glm::vec3(-0.2f, -0.8f, 0.15f)},
        {1.5f, glm::vec3(-0.2f, -0.7f, 0.0f)}
    };
    leftLegTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {0.4f, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {0.75f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {1.1f, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {1.5f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(leftLegTrack);

    auto rightLegTrack = eng::TransformTrack{"rightLeg", 5};
    rightLegTrack.positions = {
        {0.0f, glm::vec3(0.2f, -0.7f, 0.0f)},
        {0.4f, glm::vec3(0.2f, -0.8f, 0.15f)},
        {0.75f, glm::vec3(0.2f, -0.7f, 0.0f)},
        {1.1f, glm::vec3(0.2f, -0.8f, -0.15f)},
        {1.5f, glm::vec3(0.2f, -0.7f, 0.0f)}
    };
    rightLegTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {0.4f, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {0.75f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {1.1f, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {1.5f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(rightLegTrack);

    return std::move(animation);
}
