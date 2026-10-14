#include "Assets/Scene/ComponentCodec/SceneComponentCodecs.h"
#include "Assets/Scene/ComponentCodec/ComponentCodecCommon.h"
#include "Assets/Scene/SceneTypeRegistry.h"
#include "Scene/Components/RigidBodyComponent.h"

#include <rapidjson/document.h>

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace eng
{

namespace
{

using Codec::GetPayload;
using Codec::MakeParseError;
using Codec::PayloadResult;
using Codec::ReadFiniteFloat;
using Codec::ReadQuaternion;
using Codec::ReadVec3;

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
        if (sphere.radius <= 0.0f) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "sphere radius must be positive", path + ".radius");
        }
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
                } else {
                    valid = false;
                }
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
                } else {
                    valid = false;
                }
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
                } else {
                    mesh.indices.push_back(indices[i].GetUint());
                }
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
    const rapidjson::Value& value,
    const SceneParseContext& context,
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
    valid &= ReadFiniteFloat(value, "restitution", desc.restitution,
        context, path);
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
        } else {
            output = static_cast<uint16_t>(value[name].GetUint());
        }
    };
    readLayer("collisionLayer", desc.collisionLayer);
    readLayer("collisionMask", desc.collisionMask);
    if (value.HasMember("continuousCollisionDetection")) {
        if (!value["continuousCollisionDetection"].IsBool()) {
            context.ReportError(SceneLoadErrorCode::Schema,
                "continuousCollisionDetection must be boolean",
                path + ".continuousCollisionDetection");
            valid = false;
        } else {
            desc.useContinuousCollisionDetection =
                value["continuousCollisionDetection"].GetBool();
        }
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
            } else {
                valid = false;
            }
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
                    "mixed trigger and non-trigger colliders are unsupported",
                    path);
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
    if (!valid) {
        return SceneLoadResult<
            std::shared_ptr<const SceneTypePayload>>::Failure(
                MakeParseError(context, path, "invalid RigidBodyComponent"));
    }
    return PayloadResult(std::move(component));
}

bool ResolveRigidBody(const SceneTypePayload&, SceneDependencyContext&) {
    return true;
}

SceneLoadResult<std::unique_ptr<Component>> CreateRigidBody(
    const SceneTypePayload& payload,
    const ResolvedSceneAssets&,
    SceneLoadContext& context) {
    const auto* desc = GetPayload<RigidBodyComponentDesc>(payload);
    if (!desc) {
        return SceneLoadResult<std::unique_ptr<Component>>::Failure(
            context.MakeError(SceneLoadErrorCode::Construction,
                "payload type mismatch for engine.RigidBodyComponent"));
    }
    std::unique_ptr<Component> component =
        std::make_unique<RigidBodyComponent>(desc->body);
    return SceneLoadResult<std::unique_ptr<Component>>::Success(
        std::move(component));
}

} // namespace

bool RegisterRigidBodyComponentCodec(SceneTypeRegistry& registry) {
    ComponentCodec codec;
    codec.parse = ParseRigidBody;
    codec.resolve = ResolveRigidBody;
    codec.create = CreateRigidBody;
    return registry.RegisterComponent("engine.RigidBodyComponent",
        std::move(codec));
}

} // namespace eng
