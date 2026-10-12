#include "Assets/SceneComponentCodecs.h"
#include "Assets/SceneTypeRegistry.h"
#include <rapidjson/document.h>

#include <cmath>
#include <filesystem>
#include <limits>
#include <string>

namespace eng
{

namespace
{

SceneLoadError MakeError(const SceneParseContext& context,
    std::string_view path, std::string message) {
    return SceneLoadError{
        context.sourcePath, std::string(path), SceneLoadErrorCode::Schema,
        std::move(message), std::nullopt
    };
}

bool ParseAssetReference(const rapidjson::Value& value, std::string& output,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsString() || std::string(value.GetString()).empty()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "asset reference must be a non-empty string", path);
        return false;
    }
    const std::filesystem::path input(value.GetString());
    const auto normalized = input.lexically_normal();
    if (input.is_absolute() || normalized.empty() ||
        *normalized.begin() == "..") {
        context.ReportError(SceneLoadErrorCode::Schema,
            "asset reference must stay relative to the assets root", path);
        return false;
    }
    output = normalized.generic_string();
    return true;
}

bool ReadFiniteFloat(const rapidjson::Value& object, const char* name,
    float& output, const SceneParseContext& context,
    std::string_view path, bool required = false) {
    if (!object.HasMember(name)) {
        if (required) context.ReportError(SceneLoadErrorCode::Schema,
            std::string("missing number '") + name + "'",
            std::string(path) + "." + name);
        return !required;
    }
    const auto& value = object[name];
    if (!value.IsNumber() || !std::isfinite(value.GetDouble())) {
        context.ReportError(SceneLoadErrorCode::Schema,
            std::string("'") + name + "' must be a finite number",
            std::string(path) + "." + name);
        return false;
    }
    output = value.GetFloat();
    return true;
}

bool ReadInteger(const rapidjson::Value& object, const char* name,
    int& output, int minimum, const SceneParseContext& context,
    const std::string& path) {
    if (!object.HasMember(name)) return true;
    if (!object[name].IsInt() || object[name].GetInt() < minimum) {
        context.ReportError(SceneLoadErrorCode::Schema,
            std::string("'") + name + "' must be an integer >= " +
                std::to_string(minimum),
            path + "." + name);
        return false;
    }
    output = object[name].GetInt();
    return true;
}

bool ReadVec3(const rapidjson::Value& value, glm::vec3& output,
    const SceneParseContext& context, std::string_view path) {
    if (!value.IsArray() || value.Size() != 3) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "expected an array of 3 numbers", std::string(path));
        return false;
    }
    for (rapidjson::SizeType i = 0; i < 3; ++i) {
        if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "vector elements must be finite numbers",
                std::string(path) + "[" + std::to_string(i) + "]");
            return false;
        }
    }
    output = {value[0].GetFloat(), value[1].GetFloat(), value[2].GetFloat()};
    return true;
}

bool ReadQuaternion(const rapidjson::Value& value, glm::quat& output,
    const SceneParseContext& context, std::string_view path) {
    if (!value.IsArray() || value.Size() != 4) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "expected quaternion [x, y, z, w]", std::string(path));
        return false;
    }
    float parts[4]{};
    for (rapidjson::SizeType i = 0; i < 4; ++i) {
        if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble())) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "quaternion elements must be finite numbers",
                std::string(path) + "[" + std::to_string(i) + "]");
            return false;
        }
        parts[i] = value[i].GetFloat();
    }
    output = glm::quat(parts[3], parts[0], parts[1], parts[2]);
    const float length = glm::length(output);
    if (!std::isfinite(length) || length <= 0.0f) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "quaternion must have non-zero finite length", std::string(path));
        return false;
    }
    output = glm::normalize(output);
    return true;
}

template<typename T>
SceneLoadResult<std::shared_ptr<const SceneTypePayload>> Payload(T value) {
    std::shared_ptr<const SceneTypePayload> payload =
        std::make_shared<SceneTypePayloadValue<T>>(std::move(value));
    return SceneLoadResult<std::shared_ptr<const SceneTypePayload>>::Success(
        std::move(payload));
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseCamera(
    const rapidjson::Value& value, const SceneParseContext& context,
    std::string_view path) {
    CameraComponentDesc desc;
    bool valid = ReadFiniteFloat(value, "fov", desc.fov, context, path) &&
        ReadFiniteFloat(value, "near", desc.nearPlane, context, path) &&
        ReadFiniteFloat(value, "far", desc.farPlane, context, path);
    if (valid && (desc.fov <= 0.0f || desc.fov >= 180.0f ||
        desc.nearPlane <= 0.0f || desc.farPlane <= desc.nearPlane)) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "camera requires 0 < fov < 180 and 0 < near < far",
            std::string(path));
        valid = false;
    }
    if (!valid) return SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>::Failure(
            MakeError(context, path, "invalid CameraComponent"));
    return Payload(std::move(desc));
}

bool ParseCollider(const rapidjson::Value& value, ColliderDesc& desc,
    const SceneParseContext& context, const std::string& path) {
    if (!value.IsObject() || !value.HasMember("shape") ||
        !value["shape"].IsString()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "collider must be an object with string shape", path);
        return false;
    }
    const std::string shape = value["shape"].GetString();
    bool valid = true;
    if (shape == "box") {
        glm::vec3 halfExtents{0.5f};
        if (!value.HasMember("halfExtents") ||
            !ReadVec3(value["halfExtents"], halfExtents, context,
                path + ".halfExtents") ||
            halfExtents.x <= 0.0f || halfExtents.y <= 0.0f ||
            halfExtents.z <= 0.0f) {
            if (value.HasMember("halfExtents") &&
                (halfExtents.x <= 0.0f || halfExtents.y <= 0.0f ||
                 halfExtents.z <= 0.0f)) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "box halfExtents must be positive", path + ".halfExtents");
            }
            valid = false;
        }
        desc.shape = BoxShapeDesc{halfExtents};
    } else if (shape == "sphere") {
        SphereShapeDesc sphere;
        valid = ReadFiniteFloat(value, "radius", sphere.radius,
            context, path, true) && sphere.radius > 0.0f;
        if (sphere.radius <= 0.0f) context.ReportError(
            SceneLoadErrorCode::Schema, "sphere radius must be positive",
            path + ".radius");
        desc.shape = sphere;
    } else if (shape == "capsule") {
        CapsuleShapeDesc capsule;
        valid = ReadFiniteFloat(value, "radius", capsule.radius,
            context, path, true) &&
            ReadFiniteFloat(value, "height", capsule.height,
                context, path, true) &&
            capsule.radius > 0.0f && capsule.height >= 0.0f;
        if (capsule.radius <= 0.0f || capsule.height < 0.0f) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "capsule radius must be positive and height non-negative", path);
        }
        desc.shape = capsule;
    } else if (shape == "convexHull") {
        ConvexHullShapeDesc hull;
        if (!value.HasMember("points") || !value["points"].IsArray() ||
            value["points"].Size() < 4) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "convexHull requires at least 4 points", path + ".points");
            valid = false;
        } else {
            const auto& points = value["points"];
            for (rapidjson::SizeType i = 0; i < points.Size(); ++i) {
                glm::vec3 point;
                if (ReadVec3(points[i], point, context,
                    path + ".points[" + std::to_string(i) + "]")) {
                    hull.points.push_back(point);
                } else valid = false;
            }
        }
        desc.shape = std::move(hull);
    } else if (shape == "triangleMesh") {
        TriangleMeshShapeDesc mesh;
        if (!value.HasMember("vertices") || !value["vertices"].IsArray() ||
            value["vertices"].Empty()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "triangleMesh requires vertices", path + ".vertices");
            valid = false;
        } else {
            const auto& vertices = value["vertices"];
            for (rapidjson::SizeType i = 0; i < vertices.Size(); ++i) {
                glm::vec3 vertex;
                if (ReadVec3(vertices[i], vertex, context,
                    path + ".vertices[" + std::to_string(i) + "]")) {
                    mesh.vertices.push_back(vertex);
                } else valid = false;
            }
        }
        if (!value.HasMember("indices") || !value["indices"].IsArray() ||
            value["indices"].Empty() || value["indices"].Size() % 3 != 0) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "triangleMesh indices must be a non-empty triangle list",
                path + ".indices");
            valid = false;
        } else {
            const auto& indices = value["indices"];
            for (rapidjson::SizeType i = 0; i < indices.Size(); ++i) {
                if (!indices[i].IsUint() ||
                    indices[i].GetUint() >= mesh.vertices.size()) {
                    context.ReportError(SceneLoadErrorCode::Schema,
                        "triangleMesh index is out of range",
                        path + ".indices[" + std::to_string(i) + "]");
                    valid = false;
                } else mesh.indices.push_back(indices[i].GetUint());
            }
        }
        desc.shape = std::move(mesh);
    } else {
        context.ReportError(SceneLoadErrorCode::Schema,
            "unsupported collider shape '" + shape + "'", path + ".shape");
        return false;
    }
    if (value.HasMember("localPosition")) {
        valid &= ReadVec3(value["localPosition"], desc.localPosition,
            context, path + ".localPosition");
    }
    if (value.HasMember("localRotation")) {
        valid &= ReadQuaternion(value["localRotation"], desc.localRotation,
            context, path + ".localRotation");
    }
    if (value.HasMember("isTrigger")) {
        if (!value["isTrigger"].IsBool()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "isTrigger must be boolean", path + ".isTrigger");
            valid = false;
        } else {
            desc.isTrigger = value["isTrigger"].GetBool();
        }
    }
    return valid;
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseRigidBody(
    const rapidjson::Value& value, const SceneParseContext& context,
    std::string_view pathView) {
    const std::string path(pathView);
    RigidBodyComponentDesc component;
    auto& desc = component.body;
    bool valid = true;
    if (!value.HasMember("motionType") || !value["motionType"].IsString()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "motionType must be a string", path + ".motionType");
        valid = false;
    } else {
        const std::string motion = value["motionType"].GetString();
        if (motion == "static") desc.motionType = BodyMotionType::Static;
        else if (motion == "kinematic") desc.motionType = BodyMotionType::Kinematic;
        else if (motion == "dynamic") desc.motionType = BodyMotionType::Dynamic;
        else {
            context.ReportError(SceneLoadErrorCode::Schema,
                "unknown motionType '" + motion + "'", path + ".motionType");
            valid = false;
        }
    }
    valid &= ReadFiniteFloat(value, "mass", desc.mass, context, path);
    valid &= ReadFiniteFloat(value, "linearDamping", desc.linearDamping,
        context, path);
    valid &= ReadFiniteFloat(value, "angularDamping", desc.angularDamping,
        context, path);
    valid &= ReadFiniteFloat(value, "friction", desc.friction, context, path);
    valid &= ReadFiniteFloat(value, "restitution", desc.restitution, context, path);
    if (value.HasMember("gravityFactor")) {
        valid &= ReadVec3(value["gravityFactor"], desc.gravityFactor,
            context, path + ".gravityFactor");
    }
    auto readLayer = [&](const char* name, uint16_t& output) {
        if (!value.HasMember(name)) return;
        if (!value[name].IsUint() || value[name].GetUint() >
            std::numeric_limits<uint16_t>::max()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                std::string(name) + " must be an unsigned 16-bit integer",
                path + "." + name);
            valid = false;
        } else output = static_cast<uint16_t>(value[name].GetUint());
    };
    readLayer("collisionLayer", desc.collisionLayer);
    readLayer("collisionMask", desc.collisionMask);
    if (value.HasMember("continuousCollisionDetection")) {
        if (!value["continuousCollisionDetection"].IsBool()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "continuousCollisionDetection must be boolean",
                path + ".continuousCollisionDetection");
            valid = false;
        } else desc.useContinuousCollisionDetection =
            value["continuousCollisionDetection"].GetBool();
    }
    if (!value.HasMember("colliders") || !value["colliders"].IsArray() ||
        value["colliders"].Empty()) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "RigidBodyComponent requires a non-empty colliders array",
            path + ".colliders");
        valid = false;
    } else {
        const auto& colliders = value["colliders"];
        for (rapidjson::SizeType i = 0; i < colliders.Size(); ++i) {
            ColliderDesc collider;
            if (ParseCollider(colliders[i], collider, context,
                path + ".colliders[" + std::to_string(i) + "]")) {
                desc.colliders.push_back(std::move(collider));
            } else valid = false;
        }
    }
    if ((desc.motionType == BodyMotionType::Dynamic && desc.mass <= 0.0f) ||
        (desc.motionType != BodyMotionType::Dynamic && desc.mass != 0.0f) ||
        desc.linearDamping < 0.0f || desc.linearDamping > 1.0f ||
        desc.angularDamping < 0.0f || desc.angularDamping > 1.0f ||
        desc.friction < 0.0f || desc.restitution < 0.0f ||
        desc.restitution > 1.0f) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "invalid mass, damping, friction, or restitution", path);
        valid = false;
    }
    if (!desc.colliders.empty()) {
        const bool trigger = desc.colliders.front().isTrigger;
        for (const auto& collider : desc.colliders) {
            if (collider.isTrigger != trigger) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "mixed trigger and non-trigger colliders are unsupported", path);
                valid = false;
                break;
            }
        }
    }
    if (desc.motionType == BodyMotionType::Dynamic) {
        for (const auto& collider : desc.colliders) {
            if (std::holds_alternative<TriangleMeshShapeDesc>(collider.shape)) {
                context.ReportError(SceneLoadErrorCode::Schema,
                    "dynamic bodies cannot use triangleMesh colliders", path);
                valid = false;
                break;
            }
        }
    }
    if (!valid) return SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>::Failure(
            MakeError(context, path, "invalid RigidBodyComponent"));
    return Payload(std::move(component));
}

SceneLoadResult<std::shared_ptr<const SceneTypePayload>> ParseMesh(
    const rapidjson::Value& value, const SceneParseContext& context,
    std::string_view pathView) {
    const std::string path(pathView);
    MeshComponentDesc desc;
    bool valid = true;
    if (!value.HasMember("material")) {
        context.ReportError(SceneLoadErrorCode::Schema,
            "material must be an asset path", path + ".material");
        valid = false;
    } else valid &= ParseAssetReference(value["material"], desc.materialPath,
        context, path + ".material");
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
                glm::vec3 size;
                valid &= mesh.HasMember("size") &&
                    ReadVec3(mesh["size"], size, context, meshPath + ".size") &&
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
    if (!valid) return SceneLoadResult<
        std::shared_ptr<const SceneTypePayload>>::Failure(
            MakeError(context, path, "invalid MeshComponent"));
    return Payload(std::move(desc));
}

} // namespace

bool RegisterBuiltinSceneTypes(SceneTypeRegistry& registry) {
    bool success = true;
    ComponentCodec camera;
    camera.parse = ParseCamera;
    success &= registry.RegisterComponent(
        "engine.CameraComponent", std::move(camera));

    ComponentCodec rigidBody;
    rigidBody.parse = ParseRigidBody;
    success &= registry.RegisterComponent(
        "engine.RigidBodyComponent", std::move(rigidBody));

    ComponentCodec mesh;
    mesh.parse = ParseMesh;
    success &= registry.RegisterComponent(
        "engine.MeshComponent", std::move(mesh));
    return success;
}

} // namespace eng
