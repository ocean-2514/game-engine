#include "Renderer/MeshFactory.h"

#include "Renderer/RenderDevice.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace eng {

namespace {

constexpr std::size_t kFloatsPerVertex = 8; // pos(3) + normal(3) + uv(2)

uint32_t VertexCount(const std::vector<float>& vertices) {
    return static_cast<uint32_t>(vertices.size() / kFloatsPerVertex);
}

void AppendVertex(
    std::vector<float>& vertices,
    const glm::vec3& position,
    const glm::vec3& normal,
    const glm::vec2& texCoord) {
    vertices.insert(vertices.end(), {
        position.x, position.y, position.z,
        normal.x, normal.y, normal.z,
        texCoord.x, texCoord.y
    });
}

std::shared_ptr<Mesh> BuildMesh(
    RenderDevice& device,
    const std::vector<float>& vertices,
    const std::vector<uint32_t>& indices) {
    return device.CreateMesh(
        MeshFactory::GetVertexLayout(),
        vertices,
        indices);
}

void AddFace(
    std::vector<float>& vertices,
    std::vector<uint32_t>& indices,
    const glm::vec3& normal,
    const glm::vec3& p00,
    const glm::vec3& p10,
    const glm::vec3& p11,
    const glm::vec3& p01) {
    const uint32_t base = VertexCount(vertices);
    AppendVertex(vertices, p00, normal, {0.0f, 0.0f});
    AppendVertex(vertices, p10, normal, {1.0f, 0.0f});
    AppendVertex(vertices, p11, normal, {1.0f, 1.0f});
    AppendVertex(vertices, p01, normal, {0.0f, 1.0f});
    indices.insert(indices.end(), {base, base + 1, base + 2,
                                   base, base + 2, base + 3});
}

} // namespace

VertexLayout MeshFactory::GetVertexLayout() {
    VertexLayout layout{{
        {0, 3}, // position
        {1, 3}, // normal
        {2, 2}  // texCoord
    }};
    layout.Populate();
    return layout;
}

std::shared_ptr<Mesh> MeshFactory::CreatePlane(
    RenderDevice& device,
    float width,
    float height,
    int widthSegments,
    int heightSegments) {
    widthSegments = std::max(1, widthSegments);
    heightSegments = std::max(1, heightSegments);

    const float halfW = width / 2.0f;
    const float halfH = height / 2.0f;
    const float stepX = width / static_cast<float>(widthSegments);
    const float stepY = height / static_cast<float>(heightSegments);
    const int gridX1 = widthSegments + 1;
    const int gridY1 = heightSegments + 1;

    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>(gridX1) * gridY1 * kFloatsPerVertex);
    for (int iy = 0; iy < gridY1; ++iy) {
        const float y = iy * stepY - halfH;
        const float v = 1.0f - iy / static_cast<float>(heightSegments);
        for (int ix = 0; ix < gridX1; ++ix) {
            const float x = ix * stepX - halfW;
            const float u = ix / static_cast<float>(widthSegments);
            AppendVertex(vertices, {x, y, 0.0f}, {0.0f, 0.0f, 1.0f}, {u, v});
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(widthSegments) *
                    heightSegments * 6);
    for (int iy = 0; iy < heightSegments; ++iy) {
        for (int ix = 0; ix < widthSegments; ++ix) {
            const uint32_t a = static_cast<uint32_t>(ix + gridX1 * iy);
            const uint32_t b = static_cast<uint32_t>(ix + gridX1 * (iy + 1));
            const uint32_t c = static_cast<uint32_t>(ix + 1 + gridX1 * (iy + 1));
            const uint32_t d = static_cast<uint32_t>(ix + 1 + gridX1 * iy);
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }

    return BuildMesh(device, std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> MeshFactory::CreateBox(
    RenderDevice& device,
    float width,
    float height,
    float depth) {
    const float hw = width / 2.0f;
    const float hh = height / 2.0f;
    const float hd = depth / 2.0f;

    std::vector<float> vertices;
    vertices.reserve(24 * kFloatsPerVertex);
    std::vector<uint32_t> indices;
    indices.reserve(36);

    // +X
    AddFace(vertices, indices, {1.0f, 0.0f, 0.0f},
        {hw, -hh, -hd}, {hw, -hh, hd}, {hw, hh, hd}, {hw, hh, -hd});
    // -X
    AddFace(vertices, indices, {-1.0f, 0.0f, 0.0f},
        {-hw, -hh, hd}, {-hw, -hh, -hd}, {-hw, hh, -hd}, {-hw, hh, hd});
    // +Y
    AddFace(vertices, indices, {0.0f, 1.0f, 0.0f},
        {-hw, hh, -hd}, {hw, hh, -hd}, {hw, hh, hd}, {-hw, hh, hd});
    // -Y
    AddFace(vertices, indices, {0.0f, -1.0f, 0.0f},
        {-hw, -hh, hd}, {hw, -hh, hd}, {hw, -hh, -hd}, {-hw, -hh, -hd});
    // +Z
    AddFace(vertices, indices, {0.0f, 0.0f, 1.0f},
        {-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd});
    // -Z
    AddFace(vertices, indices, {0.0f, 0.0f, -1.0f},
        {hw, -hh, -hd}, {-hw, -hh, -hd}, {-hw, hh, -hd}, {hw, hh, -hd});

    return BuildMesh(device, std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> MeshFactory::CreateCube(
    RenderDevice& device,
    float size) {
    return CreateBox(device, size, size, size);
}

std::shared_ptr<Mesh> MeshFactory::CreateSphere(
    RenderDevice& device,
    float radius,
    int widthSegments,
    int heightSegments) {
    widthSegments = std::max(3, widthSegments);
    heightSegments = std::max(2, heightSegments);

    const float twoPi = glm::two_pi<float>();
    const float pi = glm::pi<float>();

    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>(widthSegments + 1) *
                     (heightSegments + 1) * kFloatsPerVertex);

    // 网格记录每一行（纬线）的顶点索引，方便后续生成三角形。
    std::vector<std::vector<uint32_t>> grid;
    grid.reserve(static_cast<std::size_t>(heightSegments) + 1);

    uint32_t index = 0;
    for (int iy = 0; iy <= heightSegments; ++iy) {
        std::vector<uint32_t> row;
        row.reserve(static_cast<std::size_t>(widthSegments) + 1);

        const float v = iy / static_cast<float>(heightSegments);
        const float theta = v * pi;

        for (int ix = 0; ix <= widthSegments; ++ix) {
            const float u = ix / static_cast<float>(widthSegments);
            const float phi = u * twoPi;

            const glm::vec3 position{
                -radius * std::cos(phi) * std::sin(theta),
                radius * std::cos(theta),
                radius * std::sin(phi) * std::sin(theta)
            };

            AppendVertex(vertices, position, glm::normalize(position), {u, 1.0f - v});
            row.push_back(index++);
        }
        grid.push_back(std::move(row));
    }

    std::vector<uint32_t> indices;
    for (int iy = 0; iy < heightSegments; ++iy) {
        for (int ix = 0; ix < widthSegments; ++ix) {
            const uint32_t a = grid[iy][ix + 1];
            const uint32_t b = grid[iy][ix];
            const uint32_t c = grid[iy + 1][ix];
            const uint32_t d = grid[iy + 1][ix + 1];

            if (iy != 0) {
                indices.insert(indices.end(), {a, b, d});
            }
            if (iy != heightSegments - 1) {
                indices.insert(indices.end(), {b, c, d});
            }
        }
    }

    return BuildMesh(device, std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> MeshFactory::CreateCylinder(
    RenderDevice& device,
    float radius,
    float height,
    int radialSegments) {
    radialSegments = std::max(3, radialSegments);

    const float twoPi = glm::two_pi<float>();
    const float halfH = height / 2.0f;

    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    auto addRing = [&](float y, const glm::vec3& normal, float uBias) {
        const uint32_t base = VertexCount(vertices);
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = i / static_cast<float>(radialSegments);
            const float phi = u * twoPi;
            const float x = radius * std::cos(phi);
            const float z = radius * std::sin(phi);
            AppendVertex(vertices, {x, y, z}, normal, {u, uBias});
        }
        return base;
    };

    // 侧面
    {
        const uint32_t sideStart = VertexCount(vertices);
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = i / static_cast<float>(radialSegments);
            const float phi = u * twoPi;
            const float x = radius * std::cos(phi);
            const float z = radius * std::sin(phi);
            const glm::vec3 normal{std::cos(phi), 0.0f, std::sin(phi)};
            AppendVertex(vertices, {x, -halfH, z}, normal, {u, 0.0f});
            AppendVertex(vertices, {x, halfH, z}, normal, {u, 1.0f});
        }
        for (int i = 0; i < radialSegments; ++i) {
            const uint32_t a = sideStart + static_cast<uint32_t>(i) * 2;
            const uint32_t b = a + 1;
            const uint32_t c = a + 2;
            const uint32_t d = a + 3;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    // 顶盖
    {
        const uint32_t center = VertexCount(vertices);
        AppendVertex(vertices, {0.0f, halfH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f});
        const uint32_t ring = addRing(halfH, {0.0f, 1.0f, 0.0f}, 0.0f);
        for (int i = 0; i < radialSegments; ++i) {
            const uint32_t a = ring + static_cast<uint32_t>(i);
            const uint32_t b = ring + static_cast<uint32_t>(i) + 1;
            indices.insert(indices.end(), {center, a, b});
        }
    }

    // 底盖
    {
        const uint32_t center = VertexCount(vertices);
        AppendVertex(vertices, {0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f});
        const uint32_t ring = addRing(-halfH, {0.0f, -1.0f, 0.0f}, 0.0f);
        for (int i = 0; i < radialSegments; ++i) {
            const uint32_t a = ring + static_cast<uint32_t>(i);
            const uint32_t b = ring + static_cast<uint32_t>(i) + 1;
            indices.insert(indices.end(), {center, b, a});
        }
    }

    return BuildMesh(device, std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> MeshFactory::CreateCone(
    RenderDevice& device,
    float radius,
    float height,
    int radialSegments) {
    radialSegments = std::max(3, radialSegments);

    const float twoPi = glm::two_pi<float>();
    const float halfH = height / 2.0f;

    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    // 侧面（每个分段使用独立顶点，以便得到正确的侧面法线）
    {
        const uint32_t sideStart = VertexCount(vertices);
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = i / static_cast<float>(radialSegments);
            const float phi = u * twoPi;
            const float x = radius * std::cos(phi);
            const float z = radius * std::sin(phi);
            const glm::vec3 normal = glm::normalize(
                glm::vec3{height * std::cos(phi), radius, height * std::sin(phi)});

            AppendVertex(vertices, {x, -halfH, z}, normal, {u, 1.0f});
            AppendVertex(vertices, {0.0f, halfH, 0.0f}, normal, {u, 0.0f});
        }
        for (int i = 0; i < radialSegments; ++i) {
            const uint32_t base0 = sideStart + static_cast<uint32_t>(i) * 2;
            const uint32_t apex = base0 + 1;
            const uint32_t base1 = sideStart + static_cast<uint32_t>(i + 1) * 2;
            indices.insert(indices.end(), {base0, base1, apex});
        }
    }

    // 底面
    {
        const uint32_t center = VertexCount(vertices);
        AppendVertex(vertices, {0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f});
        const uint32_t ring = VertexCount(vertices);
        for (int i = 0; i <= radialSegments; ++i) {
            const float u = i / static_cast<float>(radialSegments);
            const float phi = u * twoPi;
            const float x = radius * std::cos(phi);
            const float z = radius * std::sin(phi);
            AppendVertex(vertices, {x, -halfH, z}, {0.0f, -1.0f, 0.0f},
                         {0.5f + 0.5f * std::cos(phi),
                          0.5f + 0.5f * std::sin(phi)});
        }
        for (int i = 0; i < radialSegments; ++i) {
            const uint32_t a = ring + static_cast<uint32_t>(i);
            const uint32_t b = ring + static_cast<uint32_t>(i) + 1;
            indices.insert(indices.end(), {center, b, a});
        }
    }

    return BuildMesh(device, std::move(vertices), std::move(indices));
}

} // namespace eng
