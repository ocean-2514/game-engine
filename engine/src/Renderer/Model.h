#ifndef O_MODEL
#define O_MODEL

#include "Renderer/Animation.h"
#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace eng
{

class Material;
class Mesh;

struct ModelMesh {
    std::string name;
    std::shared_ptr<Mesh> mesh;
    uint32_t materialIndex = 0;
    bool skinned = false;
};

struct ModelNode {
    std::string name;
    glm::mat4 localTransform{1.0f};

    std::vector<uint32_t> meshIndices;
    std::vector<uint32_t> children;
};

struct Bone {
    uint32_t nodeIndex = 0;
    glm::mat4 inverseBindMatrix{1.0f};
};
    
class Model
{
public:
    const std::vector<ModelMesh>& GetMeshes() const;
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const;
    const std::vector<ModelNode>& GetNodes() const;
    const std::vector<Bone>& GetBones() const;
    const std::vector<std::shared_ptr<AnimationClip>>& GetAnimationClips() const;
    uint32_t GetRootNodeIndex() const;
    const std::string& GetName() const;
    void SetName(std::string name);
    const glm::mat4& GetGlobalRootInverseMat() const;
    void SetGlobalRootInverseMat(const glm::mat4& mat);
    bool IsValid() const;

    uint32_t AddModelNode(ModelNode node);
    uint32_t AddMesh(ModelMesh mesh);
    uint32_t AddBone(Bone bone);
    uint32_t GetBoneId(const std::string& name) const;
    const Bone& GetBone(uint32_t index) const;
    uint32_t FindNodeIndex(const std::string& name) const;
    void SetBoneNodeIndex(uint32_t boneIndex, uint32_t nodeIndex);
    void AddMaterial(std::shared_ptr<Material> material);
    void AddAnimationClip(std::shared_ptr<AnimationClip> clip);
    void AddBoneMapping(const std::string& name, uint32_t index);
    void SetRootNodeIndex(uint32_t index);
    
private:
    std::vector<ModelMesh> m_meshes;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<std::shared_ptr<AnimationClip>> m_animationClips;
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, uint32_t> m_boneMapping;
    std::unordered_map<std::string, uint32_t> m_nodeMapping;
    std::vector<ModelNode> m_modelNodes;
    glm::mat4 m_globalRootInverseMat{1.0f};
    uint32_t m_rootNodeIndex = 0;
    std::string m_name;

    friend class ModelAssetLoader;
};



} // namespace eng


#endif // O_MODEL
