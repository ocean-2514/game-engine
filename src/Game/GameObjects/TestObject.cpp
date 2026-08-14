#include "TestObject.h"

TestObject::TestObject(Shape shape, glm::vec3 color)
    : m_shape(shape), m_color(color) {
    auto& engine = eng::Engine::GetInstance();
    auto& device = engine.GetRenderDevice();

    switch (m_shape) {
        case Shape::Cube:
            mesh = eng::MeshFactory::CreateCone(device);
            break;
        case Shape::Sphere:
            mesh = eng::MeshFactory::CreateCylinder(device);
            break;
        case Shape::Plane:
            mesh = eng::MeshFactory::CreatePlane(device, 2.0f, 2.0f);
            break;
    }

    shader = device.CreateShaderProgram(
        std::string(SHADER_DIR) + "/vs.glsl",
        std::string(SHADER_DIR) + "/fs.glsl");
    material = std::make_shared<eng::Material>(shader);
    material->SetParam("uColor", m_color);

    AddComponent<eng::MeshComponent>(mesh, material);
}

void TestObject::OnUpdate(float deltaTime) {

}