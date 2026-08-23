#include "Renderer/Model.h"

#include "Renderer/Material.h"
#include "Renderer/Mesh.h"

#include <functional>
#include <utility>

namespace eng
{
    

const std::vector<ModelMesh>& Model::GetMeshes() const {
    return m_meshes;
}

const std::vector<std::shared_ptr<Material>>& Model::GetMaterials() const {
    return m_materials;
}   

const std::vector<ModelNode>& Model::GetNodes() const {
    return m_nodes;
}

uint32_t Model::GetRootNodeIndex() const {
    return m_rootNodeIndex;
}

const std::string& Model::GetName() const {
    return m_name;
}

void Model::SetName(std::string name) {
    m_name = std::move(name);
}

bool Model::IsValid() const {
    if (m_nodes.empty() || m_meshes.empty() ||
        m_rootNodeIndex >= m_nodes.size()) {
        return false;
    }
    for (const auto& modelMesh : m_meshes) {
        if (!modelMesh.mesh || !modelMesh.mesh->IsValid() ||
            modelMesh.materialIndex >= m_materials.size()) {
            return false;
        }
    }
    for (const auto& material : m_materials) {
        if (!material || !material->IsValid()) {
            return false;
        }
    }
    for (const auto& node : m_nodes) {
        for (const uint32_t meshIndex : node.meshIndices) {
            if (meshIndex >= m_meshes.size()) {
                return false;
            }
        }
        for (const uint32_t childIndex : node.children) {
            if (childIndex >= m_nodes.size()) {
                return false;
            }
        }
    }
    enum class VisitState : uint8_t { Unvisited, Visiting, Visited };
    std::vector<VisitState> states(m_nodes.size(), VisitState::Unvisited);
    std::function<bool(uint32_t)> visit = [&](uint32_t index) {
        if (states[index] == VisitState::Visiting) {
            return false;
        }
        if (states[index] == VisitState::Visited) {
            return true;
        }
        states[index] = VisitState::Visiting;
        for (const uint32_t childIndex : m_nodes[index].children) {
            if (!visit(childIndex)) {
                return false;
            }
        }
        states[index] = VisitState::Visited;
        return true;
    };
    if (!visit(m_rootNodeIndex)) {
        return false;
    }
    return true;
}

uint32_t Model::AddNode(ModelNode node) {
    m_nodes.push_back(std::move(node));
    return static_cast<uint32_t>(m_nodes.size() - 1);
}

uint32_t Model::AddMesh(ModelMesh mesh) {
    m_meshes.push_back(std::move(mesh));
    return static_cast<uint32_t>(m_meshes.size() - 1);
}

void Model::AddMaterial(std::shared_ptr<Material> material) {
    m_materials.push_back(std::move(material));
}

void Model::SetRootNodeIndex(uint32_t index) {
    m_rootNodeIndex = index;
}


} // namespace eng
