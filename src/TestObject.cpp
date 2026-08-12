#include "TestObject.h"

TestObject::TestObject() {
    eng::VertexLayout layout{{
        {0, 2},
        {1, 3}
    }};
    layout.Populate();

    std::vector<float> vertices = {
        -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f
    };

    auto& engine = eng::Engine::GetInstance();
    mesh = engine.GetRenderDevice().CreateMesh(layout, vertices);
    shader = engine.GetRenderDevice().CreateShaderProgram(
        std::string(SHADER_DIR) + "/vs.glsl",
        std::string(SHADER_DIR) + "/fs.glsl");
    material = std::make_shared<eng::Material>(shader);
}

void TestObject::OnUpdate(float deltaTime) {
    auto& input = eng::Engine::GetInstance().GetInputManager();
    if (input.IsKeyPressed(eng::Key::A)) {
        m_xoffset -= m_moveSpeed * deltaTime;
    } else if (input.IsKeyPressed(eng::Key::D)) {
        m_xoffset += m_moveSpeed * deltaTime;
    } 
    if (input.IsKeyPressed(eng::Key::W)) {
        m_yoffset += m_moveSpeed * deltaTime;
    } else if (input.IsKeyPressed(eng::Key::S)) {
        m_yoffset -= m_moveSpeed * deltaTime;
    }

    SetPosition(glm::vec3(m_xoffset, m_yoffset, 0.0f));

    eng::Engine::GetInstance().GetRenderQueue()
        .Submit(eng::RenderCommand{mesh, material, GetWorldTransform()});
}