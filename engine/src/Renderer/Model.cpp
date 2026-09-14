#include "Renderer/Model.h"

#include "Renderer/Material.h"
#include "Renderer/Mesh.h"

#include <functional>
#include <utility>

namespace eng
{
    
static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

const std::vector<ModelMesh>& Model::GetMeshes() const {
    return m_meshes;
}

const std::vector<std::shared_ptr<Material>>& Model::GetMaterials() const {
    return m_materials;
}   

const std::vector<ModelNode>& Model::GetNodes() const {
    return m_modelNodes;
}

const std::vector<Bone>& Model::GetBones() const {
    return m_bones;
}

const std::vector<std::shared_ptr<AnimationClip>>&
Model::GetAnimationClips() const {
    return m_animationClips;
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

const glm::mat4& Model::GetGlobalRootInverseMat() const {
    return m_globalRootInverseMat;
}

void Model::SetGlobalRootInverseMat(const glm::mat4& mat) {
    m_globalRootInverseMat = mat;
}

bool Model::IsValid() const {
    if (m_modelNodes.empty() || m_meshes.empty() ||
        m_rootNodeIndex >= m_modelNodes.size()) {
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
    for (const auto& bone : m_bones) {
        if (bone.nodeIndex >= m_modelNodes.size()) return false;
    }
    for (const auto& clip : m_animationClips) {
        if (!clip) return false;
        for (const auto& track : clip->tracks) {
            if (track.targetNodeIndex >= m_modelNodes.size()) return false;
        }
    }
    for (const auto& node : m_modelNodes) {
        for (const uint32_t meshIndex : node.meshIndices) {
            if (meshIndex >= m_meshes.size()) {
                return false;
            }
        }
        for (const uint32_t childIndex : node.children) {
            if (childIndex >= m_modelNodes.size()) {
                return false;
            }
        }
    }
    enum class VisitState : uint8_t { Unvisited, Visiting, Visited };
    std::vector<VisitState> states(m_modelNodes.size(), VisitState::Unvisited);
    std::function<bool(uint32_t)> visit = [&](uint32_t index) {
        if (states[index] == VisitState::Visiting) {
            return false;
        }
        if (states[index] == VisitState::Visited) {
            return true;
        }
        states[index] = VisitState::Visiting;
        for (const uint32_t childIndex : m_modelNodes[index].children) {
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

uint32_t Model::AddModelNode(ModelNode node) {
    m_modelNodes.push_back(std::move(node));
    const uint32_t index = static_cast<uint32_t>(m_modelNodes.size() - 1);
    m_nodeMapping.emplace(m_modelNodes.back().name, index);
    return index;
}

uint32_t Model::AddMesh(ModelMesh mesh) {
    m_meshes.push_back(std::move(mesh));
    return static_cast<uint32_t>(m_meshes.size() - 1);
}

uint32_t Model::AddBone(Bone bone) {
    m_bones.push_back(std::move(bone));
    return static_cast<uint32_t>(m_bones.size() - 1);
}

uint32_t Model::GetBoneId(const std::string& name) const {
    const auto it = m_boneMapping.find(name);
    if (it != m_boneMapping.end()) return it->second;

    return InvalidIndex;
}

const Bone& Model::GetBone(uint32_t index) const {
    return m_bones[static_cast<std::size_t>(index)];
}

uint32_t Model::FindNodeIndex(const std::string& name) const {
    const auto it = m_nodeMapping.find(name);
    return it == m_nodeMapping.end() ? InvalidIndex : it->second;
}

void Model::SetBoneNodeIndex(uint32_t boneIndex, uint32_t nodeIndex) {
    if (boneIndex < m_bones.size()) {
        m_bones[boneIndex].nodeIndex = nodeIndex;
    }
}

void Model::AddBoneMapping(const std::string& name, uint32_t index) {
    m_boneMapping.insert_or_assign(name, index);
}

void Model::AddAnimationClip(std::shared_ptr<AnimationClip> clip) {
    m_animationClips.push_back(std::move(clip));
}


void Model::AddMaterial(std::shared_ptr<Material> material) {
    m_materials.push_back(std::move(material));
}

void Model::SetRootNodeIndex(uint32_t index) {
    m_rootNodeIndex = index;
}


} // namespace eng
