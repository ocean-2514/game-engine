#include "Assets/ModelAssetLoader.h"

#include "Assets/MaterialAssetLoader.h"
#include "Assets/MaterialDesc.h"
#include "Assets/TextureAssetLoader.h"
#include "Graphics/VertexLayout.h"
#include "IO/FileSystem.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/Model.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/GltfMaterial.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eng {

namespace {

constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

struct ImportedMaterialDesc {
    std::string name;
    glm::vec3 ambient{0.0f};
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{0.0f};
    glm::vec3 emissive{0.0f};
    float shininess = 0.0f;
    float opacity = 1.0f;
    float ior = 1.0f;
    float alphaCutoff = 0.5f;
    SurfaceMode surface = SurfaceMode::Opaque;
    bool surfaceExplicit = false;
    bool twoSided = false;
    std::optional<TextureAssetDesc> diffuseTexture;
    std::optional<TextureAssetDesc> specularTexture;
    std::optional<TextureAssetDesc> normalTexture;
    std::optional<TextureAssetDesc> opacityTexture;
};

struct ImportContext {
    const aiScene& scene;
    Model& model;
    const std::filesystem::path modelDirectory;
    const ModelLoadOptions& options;
    RenderDevice& renderDevice;
    TextureAssetLoader& textureLoader;
    MaterialAssetLoader& materialLoader;
    std::unordered_map<uint32_t, uint32_t> meshIndices;
    std::unordered_map<uint32_t, uint32_t> materialIndices;
    bool failed = false;
};

glm::mat4 ToGlmMatrix(const aiMatrix4x4& value) {
    glm::mat4 result{1.0f};
    result[0][0] = value.a1;
    result[1][0] = value.a2;
    result[2][0] = value.a3;
    result[3][0] = value.a4;
    result[0][1] = value.b1;
    result[1][1] = value.b2;
    result[2][1] = value.b3;
    result[3][1] = value.b4;
    result[0][2] = value.c1;
    result[1][2] = value.c2;
    result[2][2] = value.c3;
    result[3][2] = value.c4;
    result[0][3] = value.d1;
    result[1][3] = value.d2;
    result[2][3] = value.d3;
    result[3][3] = value.d4;
    return result;
}

void PushVector(std::vector<float>& values, const aiVector3D& vector) {
    values.push_back(vector.x);
    values.push_back(vector.y);
    values.push_back(vector.z);
}

std::optional<TextureAssetDesc> ImportTextureDesc(
    const aiMaterial& material,
    aiTextureType type,
    const std::filesystem::path& modelDirectory,
    bool srgb) {
    if (material.GetTextureCount(type) == 0) {
        return std::nullopt;
    }

    aiString texturePath;
    if (material.GetTexture(type, 0, &texturePath) != AI_SUCCESS) {
        return std::nullopt;
    }
    const std::string importedPath = texturePath.C_Str();
    if (importedPath.empty()) {
        return std::nullopt;
    }
    if (importedPath.front() == '*') {
        std::cout << "ModelAssetLoader: embedded texture "
                  << importedPath << " is not supported yet\n";
        return std::nullopt;
    }

    TextureAssetDesc result;
    result.path = (modelDirectory / std::filesystem::path(importedPath))
        .lexically_normal().generic_string();
    result.loadOptions.srgb = srgb;
    // Model UVs are kept as imported; image flipping defines the engine's
    // current texture-coordinate convention.
    result.loadOptions.flipVertically = true;
    return result;
}

ImportedMaterialDesc ImportMaterial(
    const aiMaterial& material,
    const std::filesystem::path& modelDirectory) {
    ImportedMaterialDesc result;

    aiString name;
    if (material.Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        result.name = name.C_Str();
    }

    aiColor3D color;
    if (material.Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
        result.ambient = {color.r, color.g, color.b};
    }
    if (material.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        result.diffuse = {color.r, color.g, color.b};
    }
    if (material.Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        result.specular = {color.r, color.g, color.b};
    }
    if (material.Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
        result.emissive = {color.r, color.g, color.b};
    }
    material.Get(AI_MATKEY_SHININESS, result.shininess);
    material.Get(AI_MATKEY_OPACITY, result.opacity);
    material.Get(AI_MATKEY_REFRACTI, result.ior);

    aiString alphaMode;
    if (material.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
        const std::string mode = alphaMode.C_Str();
        result.surfaceExplicit = true;
        if (mode == "MASK") result.surface = SurfaceMode::Masked;
        else if (mode == "BLEND") result.surface = SurfaceMode::Transparent;
        else result.surface = SurfaceMode::Opaque;
    } else if (result.opacity < 0.999f) {
        result.surface = SurfaceMode::Transparent;
        result.surfaceExplicit = true;
    } else if (material.GetTextureCount(aiTextureType_OPACITY) > 0) {
        result.surface = SurfaceMode::Masked;
        result.surfaceExplicit = true;
    }
    material.Get(AI_MATKEY_GLTF_ALPHACUTOFF, result.alphaCutoff);
    int twoSided = 0;
    if (material.Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
        result.twoSided = twoSided != 0;
    }

    result.diffuseTexture = ImportTextureDesc(
        material, aiTextureType_DIFFUSE, modelDirectory, true);
    result.specularTexture = ImportTextureDesc(
        material, aiTextureType_SPECULAR, modelDirectory, false);
    result.normalTexture = ImportTextureDesc(
        material, aiTextureType_NORMALS, modelDirectory, false);
    if (!result.normalTexture) {
        result.normalTexture = ImportTextureDesc(
            material, aiTextureType_HEIGHT, modelDirectory, false);
    }
    result.opacityTexture = ImportTextureDesc(
        material, aiTextureType_OPACITY, modelDirectory, false);
    return result;
}

std::shared_ptr<Texture> ApplyImportedTexture(
    Material& material,
    const char* parameterName,
    const std::optional<TextureAssetDesc>& desc,
    TextureAssetLoader& textureLoader) {
    if (!desc) {
        return nullptr;
    }
    auto texture = textureLoader.LoadTexture(*desc);
    if (!texture) {
        std::cout << "ModelAssetLoader: failed to load imported texture "
                  << desc->path << "; keeping the template value\n";
        return nullptr;
    }
    auto result = texture;
    material.SetTexture(parameterName, std::move(texture));
    return result;
}

SurfaceMode ResolveTextureFallback(AlphaTextureFallback fallback) {
    switch (fallback) {
        case AlphaTextureFallback::Opaque:      return SurfaceMode::Opaque;
        case AlphaTextureFallback::Masked:      return SurfaceMode::Masked;
        case AlphaTextureFallback::Transparent: return SurfaceMode::Transparent;
    }
    return SurfaceMode::Opaque;
}

std::shared_ptr<Material> CreateMaterial(
    const aiMaterial& source,
    ImportContext& context) {
    auto material = context.materialLoader.Instantiate(
        context.options.materialTemplatePath);
    if (!material) {
        std::cout << "ModelAssetLoader: failed to instantiate material template "
                  << context.options.materialTemplatePath << '\n';
        return nullptr;
    }
    if (!context.options.importMaterials) {
        return material;
    }

    const auto imported = ImportMaterial(source, context.modelDirectory);
    const auto diffuseTexture = ApplyImportedTexture(
        *material, "uDiffuseMap", imported.diffuseTexture,
        context.textureLoader);
    const auto opacityTexture = ApplyImportedTexture(
        *material, "uOpacityMap", imported.opacityTexture,
        context.textureLoader);
    SurfaceMode surface = imported.surface;
    if (!imported.surfaceExplicit && diffuseTexture &&
        diffuseTexture->HasAlphaChannel()) {
        surface = ResolveTextureFallback(
            context.options.alphaTextureFallback);
    }
    material->SetSurfaceMode(surface);
    if (imported.twoSided) {
        RasterizerState rasterizer =
            material->GetRenderState().rasterizer;
        rasterizer.cullMode = CullMode::None;
        material->SetRasterizerState(rasterizer);
    }
    // This is the current built-in Phong material contract. Assimp semantics
    // are converted here rather than leaking uniform names into import data.
    material->SetParam("uAmbient", imported.ambient);
    material->SetParam("uDiffuse", imported.diffuse);
    material->SetParam("uSpecular", imported.specular);
    material->SetParam("uEmissive", imported.emissive);
    material->SetParam("uShininess", imported.shininess);
    material->SetParam("uOpacity", imported.opacity);
    material->SetParam("uIor", imported.ior);
    material->SetParam("uAlphaMasked",
        surface == SurfaceMode::Masked ? 1 : 0);
    material->SetParam("uAlphaCutoff", imported.alphaCutoff);
    material->SetParam("uHasOpacityMap", opacityTexture ? 1 : 0);
    ApplyImportedTexture(
        *material, "uSpecularMap", imported.specularTexture,
        context.textureLoader);
    ApplyImportedTexture(
        *material, "uNormalMap", imported.normalTexture,
        context.textureLoader);
    return material;
}

uint32_t ProcessMaterial(uint32_t sourceIndex, ImportContext& context) {
    const auto cached = context.materialIndices.find(sourceIndex);
    if (cached != context.materialIndices.end()) {
        return cached->second;
    }
    if (sourceIndex >= context.scene.mNumMaterials) {
        context.failed = true;
        return InvalidIndex;
    }

    auto material = CreateMaterial(
        *context.scene.mMaterials[sourceIndex], context);
    if (!material || !material->IsValid()) {
        context.failed = true;
        return InvalidIndex;
    }

    const auto index = static_cast<uint32_t>(
        context.model.GetMaterials().size());
    context.model.AddMaterial(std::move(material));
    context.materialIndices.emplace(sourceIndex, index);
    return index;
}

uint32_t ProcessMesh(uint32_t sourceIndex, ImportContext& context) {
    const auto cached = context.meshIndices.find(sourceIndex);
    if (cached != context.meshIndices.end()) {
        return cached->second;
    }
    if (sourceIndex >= context.scene.mNumMeshes) {
        context.failed = true;
        return InvalidIndex;
    }

    const aiMesh& source = *context.scene.mMeshes[sourceIndex];
    if (!source.HasPositions() || !source.HasNormals()) {
        std::cout << "ModelAssetLoader: mesh " << sourceIndex
                  << " has no positions or normals\n";
        context.failed = true;
        return InvalidIndex;
    }

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(static_cast<std::size_t>(source.mNumVertices) * 8);
    indices.reserve(static_cast<std::size_t>(source.mNumFaces) * 3);

    for (unsigned int i = 0; i < source.mNumVertices; ++i) {
        PushVector(vertices, source.mVertices[i]);
        PushVector(vertices, source.mNormals[i]);
        if (source.HasTextureCoords(0)) {
            vertices.push_back(source.mTextureCoords[0][i].x);
            vertices.push_back(source.mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    for (unsigned int i = 0; i < source.mNumFaces; ++i) {
        const aiFace& face = source.mFaces[i];
        if (face.mNumIndices != 3) {
            continue;
        }
        indices.insert(
            indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }
    if (indices.empty()) {
        std::cout << "ModelAssetLoader: mesh " << sourceIndex
                  << " contains no triangles\n";
        context.failed = true;
        return InvalidIndex;
    }

    VertexLayout layout{{
        {0, 3},
        {1, 3},
        {2, 2}
    }};
    layout.Populate();
    auto mesh = context.renderDevice.CreateMesh(layout, vertices, indices);
    if (!mesh || !mesh->IsValid()) {
        context.failed = true;
        return InvalidIndex;
    }

    const uint32_t materialIndex = ProcessMaterial(
        source.mMaterialIndex, context);
    if (materialIndex == InvalidIndex) {
        return InvalidIndex;
    }

    const uint32_t index = context.model.AddMesh(
        {source.mName.C_Str(), std::move(mesh), materialIndex});
    context.meshIndices.emplace(sourceIndex, index);
    return index;
}

uint32_t ProcessNode(const aiNode& source, ImportContext& context) {
    ModelNode node;
    node.name = source.mName.C_Str();
    node.localTransform = ToGlmMatrix(source.mTransformation);

    for (unsigned int i = 0; i < source.mNumMeshes; ++i) {
        const uint32_t meshIndex = ProcessMesh(source.mMeshes[i], context);
        if (meshIndex == InvalidIndex) {
            return InvalidIndex;
        }
        node.meshIndices.push_back(meshIndex);
    }
    for (unsigned int i = 0; i < source.mNumChildren; ++i) {
        const uint32_t childIndex = ProcessNode(
            *source.mChildren[i], context);
        if (childIndex == InvalidIndex) {
            return InvalidIndex;
        }
        node.children.push_back(childIndex);
    }
    return context.model.AddNode(std::move(node));
}

std::string MakeCacheKey(
    const std::string& modelPath,
    const ModelLoadOptions& options) {
    return modelPath + '\n' + options.materialTemplatePath + '\n' +
        (options.importMaterials ? "1" : "0") + '\n' +
        std::to_string(static_cast<int>(options.alphaTextureFallback));
}

} // namespace

ModelAssetLoader::ModelAssetLoader(
    const FileSystem& fileSystem,
    RenderDevice& renderDevice,
    TextureAssetLoader& textureAssetLoader,
    MaterialAssetLoader& materialAssetLoader)
    : m_fileSystem(fileSystem),
      m_renderDevice(renderDevice),
      m_textureAssetLoader(textureAssetLoader),
      m_materialAssetLoader(materialAssetLoader) {}

std::shared_ptr<Model> ModelAssetLoader::Load(
    const std::string& relativePath,
    const ModelLoadOptions& options) {
    const std::string normalizedPath =
        m_fileSystem.NormalizeAssetPath(relativePath);
    const std::string normalizedTemplate =
        m_fileSystem.NormalizeAssetPath(options.materialTemplatePath);
    if (normalizedPath.empty() || normalizedTemplate.empty()) {
        std::cout << "ModelAssetLoader: model or material template path is invalid\n";
        return nullptr;
    }

    ModelLoadOptions normalizedOptions = options;
    normalizedOptions.materialTemplatePath = normalizedTemplate;
    const std::string cacheKey = MakeCacheKey(
        normalizedPath, normalizedOptions);
    const auto cached = m_modelCache.find(cacheKey);
    if (cached != m_modelCache.end()) {
        if (auto model = cached->second.lock()) {
            return model;
        }
        m_modelCache.erase(cached);
    }

    Assimp::Importer importer;
    const auto absolutePath =
        m_fileSystem.GetAssetsFolder() / normalizedPath;
    const aiScene* scene = importer.ReadFile(
        absolutePath.string(),
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType);
    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
        std::cout << "ModelAssetLoader: "
                  << importer.GetErrorString() << '\n';
        return nullptr;
    }

    auto model = std::make_shared<Model>();
    std::string modelName = scene->mName.C_Str();
    if (modelName.empty()) {
        modelName = std::filesystem::path(normalizedPath).stem().string();
    }
    model->SetName(std::move(modelName));
    ImportContext context{
        *scene,
        *model,
        std::filesystem::path(normalizedPath).parent_path(),
        normalizedOptions,
        m_renderDevice,
        m_textureAssetLoader,
        m_materialAssetLoader};
    const uint32_t rootIndex = ProcessNode(*scene->mRootNode, context);
    if (context.failed || rootIndex == InvalidIndex) {
        return nullptr;
    }
    model->SetRootNodeIndex(rootIndex);
    if (!model->IsValid()) {
        std::cout << "ModelAssetLoader: imported model is invalid: "
                  << normalizedPath << '\n';
        return nullptr;
    }

    m_modelCache.insert_or_assign(cacheKey, model);
    return model;
}

void ModelAssetLoader::ClearCache() {
    m_modelCache.clear();
}

} // namespace eng
