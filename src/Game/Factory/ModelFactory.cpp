#include "ModelFactory.h"
#include <string>

inline std::shared_ptr<eng::AnimationClip> 
    CreateMovingAnimationClip(std::string name, float duration) {
    auto animation = std::make_shared<eng::AnimationClip>();
    animation->duration = duration;
    animation->looping = true;
    animation->name = std::move(name);

    float t1 = duration / 4.0f;
    float t2 = t1 * 2;
    float t3 = t1 * 3;

    auto leftHandTrack = eng::TransformTrack{"leftHand", 2};
    leftHandTrack.positions = {
        {0.0f, glm::vec3(-0.45f, 0.0f, 0.0f)},
        {t1, glm::vec3(-0.45f, 0.1f, -0.15f)},
        {t2, glm::vec3(-0.45f, 0.0f, 0.0f)},
        {t3, glm::vec3(-0.45f, 0.1f, 0.15f)},
        {duration, glm::vec3(-0.45f, 0.0f, 0.0f)}
    };
    leftHandTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t1, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {t2, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t3, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {duration, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(leftHandTrack);

    auto rightHandTrack = eng::TransformTrack{"rightHand", 3};
    rightHandTrack.positions = {
        {0.0f, glm::vec3(0.45f, 0.0f, 0.0f)},
        {t1, glm::vec3(0.45f, -0.1f, 0.15f)},
        {t2, glm::vec3(0.45f, 0.0f, 0.0f)},
        {t3, glm::vec3(0.45f, -0.1f, -0.15f)},
        {duration, glm::vec3(0.45f, 0.0f, 0.0f)}
    };
    rightHandTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t1, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {t2, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t3, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {duration, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(rightHandTrack);

    auto leftLegTrack = eng::TransformTrack{"leftLeg", 4};
    leftLegTrack.positions = {
        {0.0f, glm::vec3(-0.2f, -0.7f, 0.0f)},
        {t1, glm::vec3(-0.2f, -0.8f, -0.15f)},
        {t2, glm::vec3(-0.2f, -0.7f, 0.0f)},
        {t3, glm::vec3(-0.2f, -0.8f, 0.15f)},
        {duration, glm::vec3(-0.2f, -0.7f, 0.0f)}
    };
    leftLegTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t1, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {t2, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t3, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {duration, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(leftLegTrack);

    auto rightLegTrack = eng::TransformTrack{"rightLeg", 5};
    rightLegTrack.positions = {
        {0.0f, glm::vec3(0.2f, -0.7f, 0.0f)},
        {t1, glm::vec3(0.2f, -0.8f, 0.15f)},
        {t2, glm::vec3(0.2f, -0.7f, 0.0f)},
        {t3, glm::vec3(0.2f, -0.8f, -0.15f)},
        {duration, glm::vec3(0.2f, -0.7f, 0.0f)}
    };
    rightLegTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t1, glm::angleAxis(glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {t2, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {t3, glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f))},
        {duration, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(rightLegTrack);

    return animation;
}

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

    model->AddAnimationClip(CreateIdleAnimationClip());
    model->AddAnimationClip(CreateWalkingAnimationClip());
    model->AddAnimationClip(CreateRunningAnimationClip());
    model->AddAnimationClip(CreateJumpingAnimationClip());

    return model;
}

std::shared_ptr<eng::AnimationClip>
    ModelFactory::CreateWalkingAnimationClip() {
    return CreateMovingAnimationClip("Walking", 1.5f);
}

std::shared_ptr<eng::AnimationClip>
    ModelFactory::CreateRunningAnimationClip() {
    return CreateMovingAnimationClip("Running", 0.8f);
}

std::shared_ptr<eng::AnimationClip>
    ModelFactory::CreateJumpingAnimationClip() {
    auto animation = std::make_shared<eng::AnimationClip>();
    animation->duration = 1.0f;
    animation->looping = false;
    animation->name = "Jumping";

    auto bodyTrack = eng::TransformTrack{"body", 0};
    bodyTrack.positions = {
        {0.0f, glm::vec3(0.0f, 0.0f, 0.0f)},
        {0.5f, glm::vec3(0.0f, 1.0f, 0.0f)},
        {1.0f, glm::vec3(0.0f, 0.0f, 0.0f)}
    };
    animation->tracks.push_back(bodyTrack);

    return animation;
}

std::shared_ptr<eng::AnimationClip>
    ModelFactory::CreateIdleAnimationClip() {
    auto animation = std::make_shared<eng::AnimationClip>();
    animation->duration = 2.0f;
    animation->looping = true;
    animation->name = "Idle";

    auto headTrack = eng::TransformTrack{"head", 1};
    headTrack.rotations = {
        {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {0.5f, glm::angleAxis(glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f))},
        {1.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}},
        {1.5f, glm::angleAxis(glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f))},
        {2.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}
    };
    animation->tracks.push_back(headTrack);

    return animation;
}

std::shared_ptr<eng::AnimatorController>
    ModelFactory::CreatePlayerAnimatorController() {
    auto controller = std::make_shared<eng::AnimatorController>();
    const auto speedParam = controller->AddFloat("Speed", 2.0f);    // 0
    const auto jumpParam = controller->AddTrigger("Jump");             // 1

    const auto idleMotion = controller->AddClipMotion("Idle", "Idle");
    const auto walkingMotion = controller->AddClipMotion("Walking", "Walking");
    const auto runningMotion = controller->AddClipMotion("Running", "Running");
    const auto jumpingMotion = controller->AddClipMotion("Jumping", "Jumping");

    std::vector<eng::BlendTree1DChild> children = {
        {0.0f, idleMotion}, {2.0f, walkingMotion}, {4.0f, runningMotion}
    };
    const auto locomotion = controller->AddBlendTree1D(
        "Locomotion", speedParam, std::move(children));

    controller->AddState({"Locomotion", locomotion});                     // 0
    controller->AddState({"Jumping", jumpingMotion, 1.0f, false});  // 1
    controller->SetDefaultState(0); 

    eng::AnimatorTransitionDefinition locomotionToJumping{0, 1};
    locomotionToJumping.conditions.push_back({
        jumpParam, eng::AnimatorConditionOp::IsTriggered
    });
    eng::AnimatorTransitionDefinition jumpingToLocomotion{1, 0, 0.0f, true};
    controller->AddTransition(locomotionToJumping);
    controller->AddTransition(jumpingToLocomotion);

    return controller;
}
