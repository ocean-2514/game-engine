#ifndef O_MODEL
#define O_MODEL

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>

namespace eng
{

class Material;
class Mesh;

struct ModelMesh {
    std::string name;
    std::shared_ptr<Mesh> mesh;
    uint32_t materialIndex = 0;
};

struct ModelNode {
    std::string name;
    glm::mat4 localTransform{1.0f};

    std::vector<uint32_t> meshIndices;
    std::vector<uint32_t> children;
};
    
class Model
{
public:
    const std::vector<ModelMesh>& GetMeshes() const;
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const;
    const std::vector<ModelNode>& GetNodes() const;
    uint32_t GetRootNodeIndex() const;
    const std::string& GetName() const;
    void SetName(std::string name);
    bool IsValid() const;

    uint32_t AddNode(ModelNode node);
    uint32_t AddMesh(ModelMesh mesh);
    void AddMaterial(std::shared_ptr<Material> material);
    void SetRootNodeIndex(uint32_t index);
    
private:
    std::vector<ModelMesh> m_meshes;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<ModelNode> m_nodes;
    uint32_t m_rootNodeIndex = 0;
    std::string m_name;

    friend class ModelAssetLoader;
};



} // namespace eng


#endif // O_MODEL
