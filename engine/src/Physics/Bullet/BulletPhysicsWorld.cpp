#include "Physics/Bullet/BulletPhysicsWorld.h"
#include "Physics/Bullet/BulletConversions.h"

#include <cmath>
#include <algorithm>

namespace eng
{

namespace
{

struct IncludeTriggerRayResultCallback final
    : btCollisionWorld::ClosestRayResultCallback {
    bool includeTriggers = true;

    IncludeTriggerRayResultCallback(const btVector3& from,
        const btVector3& to, bool include)
        : btCollisionWorld::ClosestRayResultCallback(from, to),
          includeTriggers(include) {}

    bool needsCollision(btBroadphaseProxy* proxy) const override {
        if (!ClosestRayResultCallback::needsCollision(proxy)) return false;
        if (includeTriggers) return true;
        const auto* object = static_cast<const btCollisionObject*>(
            proxy->m_clientObject);
        return object && !(object->getCollisionFlags() &
            btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }
};

bool IsFinite(float value) {
    return std::isfinite(value);
}

bool IsFinite(const glm::quat& value) {
    return IsFinite(value.w) && IsFinite(value.x) &&
        IsFinite(value.y) && IsFinite(value.z);
}

bool HasTriangleMesh(const RigidBodyDesc& desc) {
    for (const auto& collider : desc.colliders) {
        if (std::holds_alternative<TriangleMeshShapeDesc>(collider.shape))
            return true;
    }
    return false;
}

bool IsShapeValid(const CollisionShapeDesc& desc) {
    return std::visit([](const auto& shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, BoxShapeDesc>) {
            return IsFiniteVector(shape.halfExtents) &&
                shape.halfExtents.x > 0.0f && shape.halfExtents.y > 0.0f &&
                shape.halfExtents.z > 0.0f;
        } else if constexpr (std::is_same_v<T, SphereShapeDesc>) {
            return IsFinite(shape.radius) && shape.radius > 0.0f;
        } else if constexpr (std::is_same_v<T, CapsuleShapeDesc>) {
            return IsFinite(shape.radius) && IsFinite(shape.height) &&
                shape.radius > 0.0f && shape.height >= 0.0f;
        } else if constexpr (std::is_same_v<T, ConvexHullShapeDesc>) {
            if (shape.points.size() < 4) return false;
            for (const auto& point : shape.points)
                if (!IsFiniteVector(point)) return false;
            return true;
        } else if constexpr (std::is_same_v<T, TriangleMeshShapeDesc>) {
            if (shape.vertices.empty() || shape.indices.empty() ||
                shape.indices.size() % 3 != 0) return false;
            for (const auto& vertex : shape.vertices)
                if (!IsFiniteVector(vertex)) return false;
            for (uint32_t index : shape.indices)
                if (index >= shape.vertices.size()) return false;
            return true;
        }
        return false;
    }, desc);
}

bool IsBodyDescValid(const RigidBodyDesc& desc) {
    if (desc.colliders.empty() || !IsFinite(desc.mass) ||
        !IsFinite(desc.linearDamping) || !IsFinite(desc.angularDamping) ||
        !IsFinite(desc.friction) || !IsFinite(desc.restitution) ||
        !IsFiniteVector(desc.gravityFactor) || desc.linearDamping < 0.0f ||
        desc.linearDamping > 1.0f || desc.angularDamping < 0.0f ||
        desc.angularDamping > 1.0f || desc.friction < 0.0f ||
        desc.restitution < 0.0f || desc.restitution > 1.0f ||
        (desc.motionType == BodyMotionType::Dynamic && desc.mass <= 0.0f) ||
        (desc.motionType != BodyMotionType::Dynamic && desc.mass != 0.0f) ||
        (desc.motionType == BodyMotionType::Dynamic && HasTriangleMesh(desc))) {
        return false;
    }
    const bool trigger = desc.colliders.front().isTrigger;
    for (const auto& collider : desc.colliders) {
        if (!IsFiniteVector(collider.localPosition) ||
            !IsFinite(collider.localRotation) ||
            glm::length(collider.localRotation) <= 0.0f ||
            collider.isTrigger != trigger || !IsShapeValid(collider.shape)) {
            return false;
        }
    }
    return true;
}

bool HasIdentityOffset(const ColliderDesc& collider) {
    const glm::quat rotation = glm::normalize(collider.localRotation);
    return glm::dot(collider.localPosition, collider.localPosition) <= 1e-12f &&
        std::fabs(std::fabs(rotation.w) - 1.0f) <= 1e-6f &&
        glm::dot(glm::vec3(rotation.x, rotation.y, rotation.z),
            glm::vec3(rotation.x, rotation.y, rotation.z)) <= 1e-12f;
}

bool IsRaycastDescValid(const RaycastDesc& desc) {
    return IsFiniteVector(desc.origin) && IsFiniteVector(desc.direction) &&
        glm::length(desc.direction) > 0.0f &&
        IsFinite(desc.maxDistance) && desc.maxDistance > 0.0f;
}

} // namespace


BulletPhysicsWorld::BulletPhysicsWorld(const PhysicsWorldDesc& desc) {
    m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_world = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfig.get()
    );

    m_world->setGravity(ToBtVector3(desc.gravity));
}

BulletPhysicsWorld::~BulletPhysicsWorld() {
    for (auto& slot : m_slots) {
        if (slot.alive && slot.enabled && slot.body) {
            m_world->removeRigidBody(slot.body.get());
        }
        slot.body.reset();
        slot.motionState.reset();
        slot.shapes.clear();
        slot.meshInterfaces.clear();
    }
    m_slots.clear();
}

void BulletPhysicsWorld::Simulate(float fixedDeltaTime) {
    for (auto& slot : m_slots) {
        if (slot.alive && slot.enabled && slot.body &&
            slot.motionType == BodyMotionType::Dynamic) {
            slot.previousTransform = slot.currentTransform;
        }
    }
    m_world->stepSimulation(fixedDeltaTime, 0, fixedDeltaTime);
    for (auto& slot : m_slots) {
        if (slot.alive && slot.enabled && slot.body &&
            slot.motionType == BodyMotionType::Dynamic) {
            slot.currentTransform = slot.body->getWorldTransform();
        }
    }
}

void BulletPhysicsWorld::UpdateCollisionEvents() {
    std::unordered_map<ContactPairKey, bool, ContactPairKeyHash> newContactPairs;

    int numManifolds = m_world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* manifold = m_world->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* objA = manifold->getBody0();
        const btCollisionObject* objB = manifold->getBody1();

        bool hasTouchingContact = false;
        for (int j = 0; j < manifold->getNumContacts(); ++j) {
            if (manifold->getContactPoint(j).getDistance() <= 0.0f) {
                hasTouchingContact = true;
                break;
            }
        }
        if (hasTouchingContact) {
            ContactPairKey pairKey{
                {static_cast<uint32_t>(objA->getUserIndex()), static_cast<uint32_t>(objA->getUserIndex2())},
                {static_cast<uint32_t>(objB->getUserIndex()), static_cast<uint32_t>(objB->getUserIndex2())}};
            bool isTriggerA = objA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE;
            bool isTriggerB = objB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE;
            bool isTrigger = isTriggerA || isTriggerB;
            newContactPairs.emplace(pairKey, isTrigger);

            PhysicsEventPhase phase = m_contactPairs.find(pairKey) == m_contactPairs.end() ?
                PhysicsEventPhase::Enter : PhysicsEventPhase::Stay;

            std::vector<ContactPoint> contactPoints;
            contactPoints.reserve(manifold->getNumContacts());
            for (int j = 0; j < manifold->getNumContacts(); ++j) {
                const btManifoldPoint& point = manifold->getContactPoint(j);
                if (point.getDistance() > 0.0f) continue;
                ContactPoint contactPoint;
                contactPoint.position = ToGlmVec3(point.getPositionWorldOnB());
                contactPoint.normal = ToGlmVec3(point.m_normalWorldOnB);
                contactPoint.penetration = -point.getDistance();
                contactPoint.impulse = point.m_appliedImpulse;
                contactPoints.push_back(contactPoint);
            }

            m_collisionEvents.push_back({pairKey, phase, isTrigger, std::move(contactPoints)});
        }
    }

    for (const auto& [pairKey, wasTrigger] : m_contactPairs) {
        if (newContactPairs.find(pairKey) == newContactPairs.end()) {
            m_collisionEvents.push_back(
                {pairKey, PhysicsEventPhase::Exit, wasTrigger, {}});
        }
    }

    m_contactPairs = std::move(newContactPairs);
}

void BulletPhysicsWorld::SetGravity(const glm::vec3& gravity) {
    m_world->setGravity(ToBtVector3(gravity));
    for (auto& slot : m_slots) {
        if (slot.alive && slot.body) {
            slot.body->setGravity(
                ToBtVector3(gravity * slot.gravityFactor));
        }
    }
}

glm::vec3 BulletPhysicsWorld::GetGravity() const {
    return ToGlmVec3(m_world->getGravity());
}

const std::vector<PhysicsEvent>&
    BulletPhysicsWorld::GetCollisionEvents() const {
    return m_collisionEvents;
}

void BulletPhysicsWorld::ClearCollisionEvents() {
    m_collisionEvents.clear();
}

PhysicsBodyHandle BulletPhysicsWorld::CreateBody(const RigidBodyDesc& desc,
    const glm::vec3& position, const glm::quat& rotation) {
    if (!IsBodyDescValid(desc) || !IsFiniteVector(position) ||
        !IsFinite(rotation) || glm::length(rotation) <= 0.0f) return {};

    BodySlot resources;
    btCollisionShape* shape = BuildShape(desc, resources);
    if (!shape) return {};

    btVector3 inertia(0, 0, 0);
    btScalar mass = 0.0f;
    if (desc.motionType == BodyMotionType::Dynamic) {
        mass = desc.mass;
        shape->calculateLocalInertia(mass, inertia);
    }

    btTransform transform(ToBtQuaternion(glm::normalize(rotation)),
        ToBtVector3(position));
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape, inertia);
    info.m_linearDamping = desc.linearDamping;
    info.m_angularDamping = desc.angularDamping;
    info.m_friction = desc.friction;
    info.m_restitution = desc.restitution;

    auto body = std::make_unique<btRigidBody>(info);

    btVector3 worldGravity = m_world->getGravity();
    body->setGravity(worldGravity * ToBtVector3(desc.gravityFactor));

    if (desc.useContinuousCollisionDetection) {
        btVector3 center;
        btScalar radius = 0.0f;
        shape->getBoundingSphere(center, radius);
        if (radius > SIMD_EPSILON) {
            body->setCcdMotionThreshold(radius * btScalar(0.5));
            body->setCcdSweptSphereRadius(radius * btScalar(0.2));
        }
    }
    if (desc.motionType == BodyMotionType::Kinematic) {
        body->setCollisionFlags(body->getCollisionFlags() |
            btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    if (desc.colliders.front().isTrigger) {
        body->setCollisionFlags(body->getCollisionFlags() |
            btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }

    m_world->addRigidBody(body.get(), desc.collisionLayer, desc.collisionMask);

    uint32_t index;
    if (!m_freeIndices.empty()) {
        index = m_freeIndices.back();
        m_freeIndices.pop_back();
    } else {
        index = static_cast<uint32_t>(m_slots.size());
        m_slots.emplace_back();
    }
    auto& slot = m_slots[index];
    const uint32_t generation = slot.generation;
    slot = std::move(resources);
    slot.generation = generation;
    slot.body = std::move(body);
    slot.motionState = std::move(motionState);
    slot.collisionLayer = static_cast<int16_t>(desc.collisionLayer);
    slot.collisionMask = static_cast<int16_t>(desc.collisionMask);
    slot.gravityFactor = desc.gravityFactor;
    slot.motionType = desc.motionType;
    slot.previousTransform = transform;
    slot.currentTransform = transform;
    slot.alive = true;
    slot.enabled = true;
    slot.body->setUserIndex(static_cast<int>(index));
    slot.body->setUserIndex2(static_cast<int>(slot.generation));
    return PhysicsBodyHandle{index, m_slots[index].generation};
}

void BulletPhysicsWorld::DestroyBody(PhysicsBodyHandle handle) {
    if (btRigidBody* body = GetBody(handle)) {
        auto& slot = m_slots[handle.index];
        if (slot.enabled) m_world->removeRigidBody(body);
        slot.body.reset();
        slot.motionState.reset();
        slot.shapes.clear();
        slot.meshInterfaces.clear();
        slot.generation++;
        slot.alive = false;
        slot.enabled = false;
        m_freeIndices.push_back(handle.index);
    }
}

void BulletPhysicsWorld::AddForce(PhysicsBodyHandle handle, const glm::vec3& force) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate(true);
        body->applyCentralForce(ToBtVector3(force));
    }
}

void BulletPhysicsWorld::AddImpulse(PhysicsBodyHandle handle, const glm::vec3& impulse) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate(true);
        body->applyCentralImpulse(ToBtVector3(impulse));
    }
}

void BulletPhysicsWorld::AddTorque(PhysicsBodyHandle handle, const glm::vec3& torque) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate(true);
        body->applyTorque(ToBtVector3(torque));
    }
}

void BulletPhysicsWorld::SetLinearVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate(true);
        body->setLinearVelocity(ToBtVector3(velocity));
    }
}

glm::vec3 BulletPhysicsWorld::GetLinearVelocity(PhysicsBodyHandle handle) const {
    if (btRigidBody* body = GetBody(handle)) {
        return ToGlmVec3(body->getLinearVelocity());
    }
    return glm::vec3(0.0f);
}

void BulletPhysicsWorld::SetAngularVelocity(PhysicsBodyHandle handle, const glm::vec3& velocity) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate(true);
        body->setAngularVelocity(ToBtVector3(velocity));
    }
}

glm::vec3 BulletPhysicsWorld::GetAngularVelocity(PhysicsBodyHandle handle) const {
    if (btRigidBody* body = GetBody(handle)) {
        return ToGlmVec3(body->getAngularVelocity());
    }
    return glm::vec3(0.0f);
}

void BulletPhysicsWorld::SetLocalInertia(PhysicsBodyHandle handle, 
    const glm::vec3& inertia) {
    if (btRigidBody* body = GetBody(handle)) {
        body->setMassProps(body->getMass(), ToBtVector3(inertia));
    }
}

void BulletPhysicsWorld::SetAngularFactor(PhysicsBodyHandle handle, 
    const glm::vec3& factor) {
    if (btRigidBody* body = GetBody(handle)) {
        body->setAngularFactor(ToBtVector3(factor));
    }
}

void BulletPhysicsWorld::SetWorldRotation(PhysicsBodyHandle handle, 
    const glm::quat& rotation) {
    if (btRigidBody* body = GetBody(handle)) {
        const btQuaternion bulletRotation =
            ToBtQuaternion(glm::normalize(rotation));
        btTransform transform = body->getWorldTransform();
        transform.setRotation(bulletRotation);
        body->setWorldTransform(transform);
        if (auto* motionState = body->getMotionState()) {
            motionState->setWorldTransform(transform);
        }

        auto& slot = m_slots[handle.index];
        slot.previousTransform.setRotation(bulletRotation);
        slot.currentTransform.setRotation(bulletRotation);
        m_world->updateSingleAabb(body);
        body->activate(true);
    }
}

bool BulletPhysicsWorld::Teleport(PhysicsBodyHandle handle,
    const glm::vec3& worldPosition, const glm::quat& worldRotation,
    bool clearVelocity) {
    btRigidBody* body = GetBody(handle);
    if (!body) return false;

    btTransform transform(ToBtQuaternion(glm::normalize(worldRotation)),
        ToBtVector3(worldPosition));
    body->setWorldTransform(transform);
    if (auto* motionState = body->getMotionState()) {
        motionState->setWorldTransform(transform);
    }

    if (clearVelocity) {
        body->setLinearVelocity(btVector3(0, 0, 0));
        body->setAngularVelocity(btVector3(0, 0, 0));
    }
    body->activate();
    m_world->updateSingleAabb(body);
    auto& slot = m_slots[handle.index];
    slot.previousTransform = transform;
    slot.currentTransform = transform;
    return true;
}

bool BulletPhysicsWorld::SetKinematicTransform(PhysicsBodyHandle handle,
    const glm::vec3& worldPosition,
    const glm::quat& worldRotation) {
    if (btRigidBody* body = GetBody(handle)) {
        btTransform transform(ToBtQuaternion(glm::normalize(worldRotation)),
            ToBtVector3(worldPosition));
        body->setWorldTransform(transform);
        if (auto* motionState = body->getMotionState()) {
            motionState->setWorldTransform(transform);
        }
        body->activate(true);
        m_world->updateSingleAabb(body);
        auto& slot = m_slots[handle.index];
        slot.previousTransform = transform;
        slot.currentTransform = transform;
        return true;
    }
    return false;
}

bool BulletPhysicsWorld::GetBodyTransform(PhysicsBodyHandle handle,
    glm::vec3& outWorldPosition,
    glm::quat& outWorldRotation) const {
    if (btRigidBody* body = GetBody(handle)) {
        const btTransform& transform = body->getWorldTransform();
        outWorldPosition = ToGlmVec3(transform.getOrigin());
        outWorldRotation = ToGlmQuat(transform.getRotation());
        return true;
    }
    return false;
}

bool BulletPhysicsWorld::GetInterpolatedBodyTransform(
    PhysicsBodyHandle handle, float alpha,
    glm::vec3& outWorldPosition,
    glm::quat& outWorldRotation) const {
    if (!GetBody(handle)) return false;
    const auto& slot = m_slots[handle.index];
    const btVector3 position = slot.previousTransform.getOrigin().lerp(
        slot.currentTransform.getOrigin(), alpha);
    const btQuaternion rotation = slot.previousTransform.getRotation().slerp(
        slot.currentTransform.getRotation(), alpha);
    outWorldPosition = ToGlmVec3(position);
    outWorldRotation = glm::normalize(ToGlmQuat(rotation));
    return true;
}

bool BulletPhysicsWorld::RaycastClosest(
    const RaycastDesc& query, RaycastHit& hit) const {
    if (!IsRaycastDescValid(query)) return false;
    btVector3 from = ToBtVector3(query.origin);
    glm::vec3 direction = glm::normalize(query.direction);
    btVector3 to = ToBtVector3(query.origin + direction * query.maxDistance);
    IncludeTriggerRayResultCallback callback{from, to, query.includeTriggers};
    callback.m_collisionFilterMask = query.collisionMask;
    m_world->rayTest(from, to, callback);
    if (callback.hasHit()) {
        uint32_t index = static_cast<uint32_t>(callback.m_collisionObject->getUserIndex());
        uint32_t generation = static_cast<uint32_t>(callback.m_collisionObject->getUserIndex2());
        if (index >= m_slots.size()) return false;
        if (!m_slots[index].alive || m_slots[index].generation != generation) return false;

        hit.body = PhysicsBodyHandle{index, generation};
        hit.point = ToGlmVec3(callback.m_hitPointWorld);
        hit.normal = ToGlmVec3(callback.m_hitNormalWorld);
        hit.distance = callback.m_closestHitFraction * query.maxDistance;
        hit.fraction = callback.m_closestHitFraction;
        return true;
    }
    return false;
}

void BulletPhysicsWorld::WakeUp(PhysicsBodyHandle handle) {
    if (btRigidBody* body = GetBody(handle)) {
        body->activate();
    }
}

void BulletPhysicsWorld::SetBodyEnabled(PhysicsBodyHandle handle, bool enabled) {
    btRigidBody* body = GetBody(handle);
    if (!body) return;
    auto& slot = m_slots[handle.index];
    if (slot.enabled == enabled) return;
    if (enabled) {
        m_world->addRigidBody(body, slot.collisionLayer, slot.collisionMask);
        body->activate(true);
    } else {
        m_world->removeRigidBody(body);
    }
    slot.enabled = enabled;
}

btCollisionShape* BulletPhysicsWorld::BuildShape(
    const RigidBodyDesc& desc, BodySlot& slot) {
    auto createShape = [&slot](const CollisionShapeDesc& shapeDesc)
        -> btCollisionShape* {
        return std::visit([&slot](const auto& value) -> btCollisionShape* {
            using T = std::decay_t<decltype(value)>;
            std::unique_ptr<btCollisionShape> shape;
            if constexpr (std::is_same_v<T, BoxShapeDesc>) {
                shape = std::make_unique<btBoxShape>(ToBtVector3(value.halfExtents));
            } else if constexpr (std::is_same_v<T, SphereShapeDesc>) {
                shape = std::make_unique<btSphereShape>(value.radius);
            } else if constexpr (std::is_same_v<T, CapsuleShapeDesc>) {
                shape = std::make_unique<btCapsuleShape>(value.radius, value.height);
            } else if constexpr (std::is_same_v<T, ConvexHullShapeDesc>) {
                auto hull = std::make_unique<btConvexHullShape>();
                for (const auto& point : value.points)
                    hull->addPoint(ToBtVector3(point), false);
                hull->recalcLocalAabb();
                shape = std::move(hull);
            } else if constexpr (std::is_same_v<T, TriangleMeshShapeDesc>) {
                auto mesh = std::make_unique<btTriangleMesh>();
                for (std::size_t i = 0; i < value.indices.size(); i += 3) {
                    mesh->addTriangle(
                        ToBtVector3(value.vertices[value.indices[i]]),
                        ToBtVector3(value.vertices[value.indices[i + 1]]),
                        ToBtVector3(value.vertices[value.indices[i + 2]]));
                }
                auto* meshPtr = mesh.get();
                slot.meshInterfaces.push_back(std::move(mesh));
                shape = std::make_unique<btBvhTriangleMeshShape>(meshPtr, true);
            }
            if (!shape) return nullptr;
            btCollisionShape* result = shape.get();
            slot.shapes.push_back(std::move(shape));
            return result;
        }, shapeDesc);
    };

    if (desc.colliders.size() == 1 && HasIdentityOffset(desc.colliders.front()))
        return createShape(desc.colliders.front().shape);

    auto compound = std::make_unique<btCompoundShape>();
    for (const auto& collider : desc.colliders) {
        btCollisionShape* child = createShape(collider.shape);
        if (!child) return nullptr;
        const btTransform localTransform(
            ToBtQuaternion(glm::normalize(collider.localRotation)),
            ToBtVector3(collider.localPosition));
        compound->addChildShape(localTransform, child);
    }
    btCollisionShape* result = compound.get();
    slot.shapes.push_back(std::move(compound));
    return result;
}

btRigidBody* BulletPhysicsWorld::GetBody(
    PhysicsBodyHandle handle) const {
    if (handle.index >= m_slots.size()) return nullptr;
    const auto& slot = m_slots[handle.index];
    if (!slot.alive || slot.generation != handle.generation) return nullptr;
    return slot.body.get();
}


} // namespace eng
