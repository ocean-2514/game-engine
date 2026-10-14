#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Assets/AssetManager.h"
#include "IO/FileSystem.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Scene/Components/MeshComponent.h"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <utility>

namespace eng
{

namespace
{

using Codec::GetPayload;
using Codec::MakeParseError;
using Codec::ParseAssetReference;
using Codec::PayloadResult;
using Codec::ReadFiniteFloat;
using Codec::ReadInteger;
using Codec::ReadVec3;

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseMesh(
    const rapidjson::Value& value,
    const SceneParseContext& context,
    std::string_view pathView) {
    const std::string path(pathView);
    MeshComponentDesc desc;
    bool valid = true;
    if (!value.HasMember("material")) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "material must be an asset path", path + ".material");
        valid = false;
    } else {
        valid &= ParseAssetReference(value["material"], desc.materialPath,
            context, path + ".material");
    }
    if (!value.HasMember("mesh") || !value["mesh"].IsObject()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "mesh must be an object", path + ".mesh");
        valid = false;
    } else {
        const auto& mesh = value["mesh"];
        if (!mesh.HasMember("kind") || !mesh["kind"].IsString() ||
            std::string(mesh["kind"].GetString()) != "primitive" ||
            !mesh.HasMember("primitive") || !mesh["primitive"].IsString()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "mesh must describe a primitive", path + ".mesh");
            valid = false;
        } else {
            const std::string primitive = mesh["primitive"].GetString();
            const std::string meshPath = path + ".mesh";
            if (primitive == "cube") {
                CubeMeshDesc shape;
                valid &= ReadFiniteFloat(mesh, "size", shape.size,
                    context, meshPath, true) && shape.size > 0.0f;
                desc.shape = shape;
            } else if (primitive == "box") {
                glm::vec3 size{1.0f};
                valid &= mesh.HasMember("size") &&
                    ReadVec3(mesh["size"], size, context,
                        meshPath + ".size") &&
                    size.x > 0.0f && size.y > 0.0f && size.z > 0.0f;
                desc.shape = BoxMeshDesc{size.x, size.y, size.z};
            } else if (primitive == "plane") {
                PlaneMeshDesc shape;
                valid &= ReadFiniteFloat(mesh, "width", shape.width,
                    context, meshPath, true) &&
                    ReadFiniteFloat(mesh, "height", shape.height,
                        context, meshPath, true) &&
                    shape.width > 0.0f && shape.height > 0.0f;
                valid &= ReadInteger(mesh, "widthSegments", shape.widthSegments,
                    1, context, meshPath);
                valid &= ReadInteger(mesh, "heightSegments", shape.heightSegments,
                    1, context, meshPath);
                desc.shape = shape;
            } else if (primitive == "sphere") {
                SphereMeshDesc shape;
                valid &= ReadFiniteFloat(mesh, "radius", shape.radius,
                    context, meshPath, true) && shape.radius > 0.0f;
                valid &= ReadInteger(mesh, "widthSegments", shape.widthSegments,
                    3, context, meshPath);
                valid &= ReadInteger(mesh, "heightSegments", shape.heightSegments,
                    2, context, meshPath);
                desc.shape = shape;
            } else if (primitive == "cylinder") {
                CylinderMeshDesc shape;
                valid &= ReadFiniteFloat(mesh, "radius", shape.radius,
                    context, meshPath, true) && shape.radius > 0.0f;
                valid &= ReadFiniteFloat(mesh, "height", shape.height,
                    context, meshPath, true) && shape.height > 0.0f;
                valid &= ReadInteger(mesh, "radialSegments", shape.radialSegments,
                    3, context, meshPath);
                desc.shape = shape;
            } else if (primitive == "cone") {
                ConeMeshDesc shape;
                valid &= ReadFiniteFloat(mesh, "radius", shape.radius,
                    context, meshPath, true) && shape.radius > 0.0f;
                valid &= ReadFiniteFloat(mesh, "height", shape.height,
                    context, meshPath, true) && shape.height > 0.0f;
                valid &= ReadInteger(mesh, "radialSegments", shape.radialSegments,
                    3, context, meshPath);
                desc.shape = shape;
            } else {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "unsupported primitive '" + primitive + "'",
                    meshPath + ".primitive");
                valid = false;
            }
        }
    }
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid MeshComponent"));
    }
    return PayloadResult(std::move(desc));
}

bool ResolveMesh(const SceneTypePayload& payload,
    SceneDependencyContext& context) {
    const auto* desc = GetPayload<MeshComponentDesc>(payload);
    if (!desc) {
        context.ReportError(SceneLoadErrorCode::Construction,
            "payload type mismatch for engine.MeshComponent");
        return false;
    }

    bool success = true;
    std::shared_ptr<Mesh> mesh = context.renderDevice.CreateMesh(desc->shape);
    if (!mesh) {
        success = false;
        context.ReportError(SceneLoadErrorCode::Construction,
            "failed to create primitive mesh");
    } else {
        context.output.Store(SceneAssetKey{context.jsonPath, "mesh"}, mesh);
    }

    const std::string materialPath =
        context.fileSystem.NormalizeAssetPath(desc->materialPath);
    if (materialPath.empty()) {
        context.ReportError(SceneLoadErrorCode::AssetNotFound,
            "material asset was not found or is outside the assets root",
            desc->materialPath);
        return false;
    }
    std::shared_ptr<Material> material =
        context.assets.LoadMaterial(materialPath);
    if (!material) {
        success = false;
        context.ReportError(SceneLoadErrorCode::AssetNotFound,
            "failed to load material", materialPath);
    } else {
        context.output.Store(SceneAssetKey{context.jsonPath, "material"},
            material);
    }
    return success;
}

SceneLoadResult<std::unique_ptr<Component>> CreateMesh(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets& assets,
    SceneLoadContext& context) {
    if (!GetPayload<MeshComponentDesc>(payload)) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.MeshComponent"));
    }
    auto mesh = assets.Find<Mesh>(SceneAssetKey{context.jsonPath, "mesh"});
    auto material = assets.Find<Material>(
        SceneAssetKey{context.jsonPath, "material"});
    if (!mesh || !material) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "resolved mesh or material dependency is missing"));
    }
    std::unique_ptr<Component> component =
        std::make_unique<MeshComponent>(std::move(mesh), std::move(material));
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

} // namespace

bool RegisterMeshComponentCodec(SceneTypeRegistry& registry) {
    ComponentCodec codec;
    codec.parse = ParseMesh;
    codec.resolve = ResolveMesh;
    codec.create = CreateMesh;
    return registry.RegisterComponent("engine.MeshComponent",
        std::move(codec));
}

} // namespace eng
