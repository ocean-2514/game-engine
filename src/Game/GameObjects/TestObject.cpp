#include "TestObject.h"
#include <iostream>
TestObject::TestObject(
    Shape shape,
    glm::vec3 color,
    eng::AssetManager& assetManager)
    : m_shape(shape), m_color(color) {
    auto& engine = eng::Engine::GetInstance();
    auto& device = engine.GetRenderDevice();

    switch (m_shape) {
        case Shape::Cube:
            mesh = eng::MeshFactory::CreateCube(device);
            break;
        case Shape::Sphere:
            mesh = eng::MeshFactory::CreateSphere(device);
            break;
        case Shape::Plane:
            mesh = eng::MeshFactory::CreatePlane(device, 2.0f, 2.0f);
            break;
    }

    material = assetManager.LoadMaterial("material/container2.json");
    // material->SetParam("uHasDiffuseMap", 1); 

    if (mesh && material) {
        auto* meshComp = AddComponent<eng::MeshComponent>(mesh, material);
    }
}

void TestObject::OnUpdate(float deltaTime) {

}
