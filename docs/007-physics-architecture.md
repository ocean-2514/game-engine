# 物理系统架构

- 状态：第一阶段基础链路已实现，持续迭代
- 后端：Bullet Physics（Bullet 2 API）
- 最后更新：2026-10-05

## 目标与第一阶段范围

物理系统第一阶段的目标是形成一条稳定的最小链路：Scene 拥有独立物理世界，GameObject 通过组件创建刚体与碰撞体，物理世界按固定步长模拟，动态刚体把结果同步回场景变换，并能进行 Raycast、Trigger 与基础碰撞事件查询。

第一阶段建议支持：

- Static、Kinematic、Dynamic 三种刚体运动类型；
- Box、Sphere、Capsule、ConvexHull 和静态 TriangleMesh 形状；
- 重力、质量、阻尼、速度、力、冲量和 Teleport；
- 碰撞层与碰撞掩码；
- Collision/Trigger Enter、Stay、Exit；
- Raycast 和简单的 Overlap 查询；
- 可选的 Bullet Debug Draw。

第一阶段暂不实现软体、车辆、多刚体系统、布料、复杂约束、连续角色控制器、物理材质资产、异步物理和多物理后端。角色移动可以先使用普通动态/运动学刚体验证链路，等需求明确后再决定使用 `btKinematicCharacterController`，还是自定义基于 Sweep 的 CharacterController。

## 总体边界

Bullet 是当前确定的实现后端，但公开引擎接口不应直接暴露 `btRigidBody`、`btCollisionShape`、`btVector3` 等类型。这样做的主要目的不是立即支持另一套物理库，而是：

- 隔离 Bullet 头文件和编译依赖；
- 集中处理 GLM/Bullet 坐标转换和生命周期顺序；
- 防止游戏代码绕过 Scene 阶段直接修改 Bullet World；
- 让组件销毁、事件派发和物理查询遵守引擎自己的规则；
- 以后升级 Bullet、替换具体形状实现或增加测试替身时，不需要修改 GameObject API。

建议的依赖方向：

```text
Game / Script
  -> RigidBodyComponent / Collider 描述 / PhysicsWorld 查询接口
  -> Scene
  -> PhysicsWorld
  -> BulletPhysicsWorld（或 PhysicsWorld.cpp 内部 Impl）
  -> Bullet
```

当前没有多后端需求，因此不必先创建 `IPhysicsBackend`、`BulletRigidBody` 等一整套虚接口。可以让 `PhysicsWorld` 使用 PImpl：头文件只声明引擎类型，`PhysicsWorld.cpp` 或 `Physics/Bullet/` 下的文件持有 Bullet 对象。等第二种后端真正出现后，再从已有稳定边界提取接口。

## 建议的目录与文件

```text
engine/src/Physics/
├─ PhysicsTypes.h                 公共枚举、Desc、查询和事件值类型
├─ CollisionShape.h/.cpp         只读共享形状资源（可在第二步加入）
├─ PhysicsWorld.h/.cpp           Scene 级世界、模拟、查询、事件
└─ Bullet/
   ├─ BulletPhysicsWorld.h/.cpp  Bullet 世界对象与 Body Registry
   ├─ BulletConversions.h        GLM 与 Bullet 类型转换，仅后端可见
   └─ BulletDebugDrawer.h/.cpp   可选调试绘制

engine/src/Scene/Components/
├─ RigidBodyComponent.h/.cpp     GameObject 与 Physics Body 的桥梁
└─ ColliderComponent.h/.cpp      可选；见下文组件组织
```

如果第一版希望文件更少，可以先让 `PhysicsWorld` 内部直接使用私有 `Impl`，不单独创建 `BulletPhysicsWorld`。但 Bullet include 应尽量只出现在 `.cpp` 和 `Physics/Bullet/` 内。

## Scene 与 PhysicsWorld 的所有权

每个 `Scene` 应拥有一个 `PhysicsWorld`，而不是让 Engine 持有全局单例：

```text
Scene
├─ GameObject tree
├─ PhysicsWorld
└─ Render-related scene state
```

原因是不同 Scene 可能需要不同重力、暂停状态、时间倍率或独立模拟；测试 Scene 也不应污染主游戏世界。Engine 目前并不拥有“主 Scene”，由 Scene 自己驱动物理也更符合现有结构。

建议接口：

```cpp
struct PhysicsWorldDesc {
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    float fixedTimeStep = 1.0f / 60.0f;
    uint32_t maxSubStepsPerFrame = 8;
    float maxAccumulatedTime = 0.25f;
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(const PhysicsWorldDesc& desc = {});
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void Simulate(float fixedDeltaTime);
    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

    bool RaycastClosest(const RaycastDesc& query, RaycastHit& hit) const;
    std::vector<RaycastHit> RaycastAll(const RaycastDesc& query) const;
};
```

Scene 必须保证 PhysicsWorld 比物理组件活得更久。若两者直接作为成员，声明顺序要让 GameObject 树先析构、PhysicsWorld 后析构；更直观的做法是在 `Scene::~Scene()` 中先清空对象，再销毁 PhysicsWorld。`Scene::Clear()` 也必须让所有 Body 从 World 注销，不能只清空 GameObject 而留下 Bullet user pointer。

## Bullet World 的内部所有权

Bullet 世界最小配置通常包含：

```text
btDefaultCollisionConfiguration
btCollisionDispatcher
btBroadphaseInterface
btSequentialImpulseConstraintSolver
btDiscreteDynamicsWorld
```

它们存在析构顺序要求。建议放在 `PhysicsWorld::Impl` 中，以构造的反序安全销毁，并显式保证：

1. 先从 dynamics world 移除并销毁所有 rigid body / collision object；
2. 再销毁 motion state；
3. 再释放 collision shape 的拥有引用；
4. 最后销毁 dynamics world、solver、broadphase、dispatcher 和 configuration。

不要让 `RigidBodyComponent` 独立拥有一个仍注册在已销毁 World 中的 `btRigidBody`。更稳妥的方式是 PhysicsWorld/Impl 统一拥有后端 Body，组件只保存引擎句柄：

```cpp
struct PhysicsBodyHandle {
    uint32_t index = InvalidIndex;
    uint32_t generation = 0;
};
```

第一版若暂时使用非拥有 Body 指针，也必须让组件析构同步注销，并严格保证 PhysicsWorld 最后析构。由于现有 SceneCommand、相机和组件仍大量使用观察裸指针，短期可以先实现后者；但碰撞事件可能延迟到 Step 之后派发，Body Handle 会明显更安全，值得优先采用。

## 组件组织

### 推荐的第一版：RigidBodyComponent 持有形状描述

当前 `GameObject::GetComponent<T>()` 是精确类型并只返回一个组件，不适合立刻设计多个派生 Collider 组件。第一版可以让一个 `RigidBodyComponent` 持有一个或多个形状描述：

```cpp
enum class BodyMotionType {
    Static,
    Kinematic,
    Dynamic
};

struct BoxShapeDesc {
    glm::vec3 halfExtents{0.5f};
};

struct SphereShapeDesc {
    float radius = 0.5f;
};

struct CapsuleShapeDesc {
    float radius = 0.5f;
    float height = 1.0f; // 不含两端半球时必须在文档中明确
};

using CollisionShapeDesc = std::variant<
    BoxShapeDesc,
    SphereShapeDesc,
    CapsuleShapeDesc,
    ConvexHullShapeDesc,
    TriangleMeshShapeDesc>;

struct ColliderDesc {
    CollisionShapeDesc shape;
    glm::vec3 localPosition{0.0f};
    glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    bool isTrigger = false;
};

struct RigidBodyDesc {
    BodyMotionType motionType = BodyMotionType::Static;
    float mass = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    glm::vec3 gravityFactor{1.0f};
    uint16_t collisionLayer = 1;
    uint16_t collisionMask = 0xFFFF;
    bool useContinuousCollisionDetection = false;
    std::vector<ColliderDesc> colliders;
};
```

单个 Collider 可以直接创建一个 Bullet shape；多个 Collider 使用 `btCompoundShape`。形状的 local offset 属于 Collider，不应通过额外 GameObject 子节点暗中表达。

建议约束：

- Dynamic：`mass > 0`；
- Static/Kinematic：Bullet mass 固定为 0；
- Dynamic 至少有一个非 Trigger Collider；
- TriangleMesh 只允许 Static；运动的凹多边形成本高且行为受限；
- Dynamic Mesh 优先生成 ConvexHull 或离线 convex decomposition；
- 空 shape、负尺寸、NaN/Inf、非法质量在创建时直接拒绝。

### 何时拆分 ColliderComponent

当出现以下真实需求时，再拆出独立 `ColliderComponent`：

- 一个 GameObject 需要动态增删多个 Collider；
- Collider 需要单独 enabled、材质、事件过滤或编辑器 Gizmo；
- Collider 与 RigidBody 位于不同层级节点；
- 纯 Trigger 对象不希望表现成 RigidBody。

拆分后仍不建议用 `BoxColliderComponent`、`SphereColliderComponent` 等大量派生类。一个 `ColliderComponent` 持有 `CollisionShapeDesc` 更符合当前精确 TypeId 查询；同时需要给 GameObject 增加 `GetComponents<T>()`，或明确第一版每对象一个 Collider。

## RigidBodyComponent 的职责

组件是游戏侧门面，不应执行 Bullet broadphase 或保存整个 PhysicsWorld：

```cpp
class RigidBodyComponent : public Component {
public:
    ENG_COMPONENT_TYPE(RigidBodyComponent);

    void AddForce(const glm::vec3& force);
    void AddImpulse(const glm::vec3& impulse);
    void AddTorque(const glm::vec3& torque);
    void SetLinearVelocity(const glm::vec3& velocity);
    glm::vec3 GetLinearVelocity() const;
    void SetAngularVelocity(const glm::vec3& velocity);
    void Teleport(const glm::vec3& worldPosition,
                  const glm::quat& worldRotation,
                  bool clearVelocity = false);
    void WakeUp();
    void SetEnabled(bool enabled);

private:
    PhysicsWorld* m_world = nullptr;       // 非拥有，Scene 保证生命周期
    PhysicsBodyHandle m_body;
};
```

`AddForce` 表示持续力，通常每个 fixed step 重复施加；`AddImpulse` 表示瞬时冲量。接口命名应明确是否在世界空间还是局部空间，例如 `AddForceWorld` / `AddForceLocal`，不要靠调用者猜测。

组件不应允许用户取得可写 `btRigidBody*`。如果调试确实需要，可在 Bullet 后端内部提供断言或诊断接口，而不是成为公开契约。

## 组件注册与注销

当前已经统一增加 `Component::OnAttach(Scene&)` / `OnDetach(Scene&)` 和 GameObject 的所属 Scene 观察指针。组件加到已挂接对象时立即 Attach；对象连同预先创建的组件接入 Scene 时递归 Attach。组件实际删除、GameObject 子树销毁或 Scene Clear 时递归 Detach，并保证回调发生在 owner 和 PhysicsWorld 仍有效时。重新设置同一 Scene 内的 parent 不触发 Detach/Attach。

`RigidBodyComponent` 构造函数只保存 `RigidBodyDesc`，`OnAttach` 从 Scene 取得 PhysicsWorld 并创建 Body，`OnDetach` 先销毁 Body、使 Handle 失效，再清空 World 观察指针。这样游戏侧创建保持为：

```cpp
object->AddComponent<RigidBodyComponent>(bodyDesc);
```

不能在 Component 构造函数中读取 `m_owner`，因为 `AddComponent<T>()` 是先构造、后 `AttachComponent()`，此时 owner 仍为空。也不要只依靠普通 `OnUpdate()` 注册，否则 disabled/未更新对象可能永远不进入物理世界。

当前 Attach/Detach 都发生在 Bullet Step 之外，可以直接注册和注销。加入碰撞事件回调后，注册和注销操作应进入 PhysicsWorld 自己的 pending command 队列，在物理 Step/事件派发前后安全点执行；Bullet 正在执行碰撞检测或遍历 manifold 时不能直接增删 Body。

## 固定时间步与 Scene 更新流程

物理模拟不应直接使用每帧变化的 `deltaTime`。Scene 保存 accumulator，并使用固定步长：

```cpp
accumulator += min(frameDelta, maxAccumulatedTime);

FlushPhysicsCommands();
while (accumulator >= fixedTimeStep && steps < maxSubStepsPerFrame) {
    FixedUpdateTree(fixedTimeStep);        // 脚本施加力/设置速度
    PushKinematicTransforms();             // Scene -> Physics
    PhysicsWorld::Simulate(fixedTimeStep);
    PullDynamicTransforms();               // Physics -> Scene
    CollectPhysicsEvents();
    accumulator -= fixedTimeStep;
}
DispatchPhysicsEvents();
UpdateTree(frameDelta);                    // 动画、相机和普通帧逻辑
FlushSceneAndComponentCommands();
```

由于外层已经有 accumulator，调用 Bullet 时建议：

```cpp
dynamicsWorld->stepSimulation(fixedTimeStep, 0, fixedTimeStep);
```

不要同时让 Scene 和 Bullet 都执行子步累积，否则会形成两套时钟并难以预测。`frameDelta` 应限制最大值，`maxSubStepsPerFrame` 防止断点恢复或卡顿后陷入“追赶螺旋”。超过预算的累计时间是截断、保留还是记录警告，需要固定一种策略；第一版可以截断并输出限频诊断。

当前 `GameObject` 只有 `OnUpdate()`。建议增加不可由外部直接调用的：

```cpp
Component::OnFixedUpdate(float fixedDeltaTime);
GameObject::OnFixedUpdate(float fixedDeltaTime);
GameObject::FixedUpdateTree(float fixedDeltaTime);
```

玩家输入可以在普通 Update 中写入“移动意图”，物理控制组件在 FixedUpdate 中读取意图并施加力。这样一次渲染帧包含多个物理子步时，控制逻辑仍按固定频率执行。

## Transform 的控制权

物理与 GameObject 双向写变换是最容易产生抖动和反馈循环的地方，必须按 Body 类型规定唯一控制者：

| Body 类型 | 模拟期间变换来源 | 同步方向 |
|---|---|---|
| Static | 创建时的 GameObject 变换 | Scene -> Physics，仅显式更新 |
| Kinematic | 游戏/动画设置的 GameObject 变换 | 每个 fixed step：Scene -> Physics |
| Dynamic | Bullet 模拟结果 | 每个 fixed step：Physics -> Scene |

Dynamic 对象创建后，普通 `SetPosition/SetRotation` 不应被当作日常移动方式。瞬移必须通过 `RigidBodyComponent::Teleport()`，同时更新 Bullet transform、motion state、AABB、激活状态，并按参数决定是否清除速度。

Static 对象移动后也要显式通知物理世界刷新 broadphase；因此后续可以增加 GameObject transform dirty version。第一版没有 dirty 标志时，可以由 `RigidBodyComponent::SetStaticTransform()` 或 Teleport 明确更新，避免每帧比较矩阵。

### 父子层级约束

Bullet 刚体使用世界空间，而 GameObject 保存局部 TRS。动态刚体同步回场景时需要：

```text
localTransform = inverse(parentWorldTransform) * physicsWorldTransform
```

但父节点的非均匀缩放、运动父节点和动态子刚体组合会产生难以解释的速度、惯性和碰撞形状缩放。第一版建议明确限制：

- Dynamic RigidBody 应放在 Scene 根节点，或父节点只能是静止、单位缩放的组织节点；
- 一个刚体层级只由刚体根节点参与物理，视觉 Mesh 可以作为它的子节点；
- 不把骨骼节点或动画写入的节点同时设为 Dynamic RigidBody；
- Kinematic 可以由动画/脚本驱动，但不接受物理反向覆盖；
- 运行时修改碰撞体缩放需要重建/更新 shape 和惯性，第一版可以禁止。

这与现有 `AnimationComponent` 写 GameObject 子节点姿态的流程相容：角色整体物理 Body 放在模型实例根节点，骨骼动画只写其下面的视觉骨架。

## 渲染插值

固定 60 Hz 的物理直接写 GameObject，而渲染可能是 144 Hz，会看到轻微阶梯。当前实现已经为每个 Dynamic Body 保存：

```text
previousPhysicsTransform
currentPhysicsTransform
alpha = accumulator / fixedTimeStep
renderTransform = interpolate(previous, current, alpha)
```

位置使用线性插值，旋转使用四元数 `slerp`。`Teleport()` 会同时重置 previous/current，防止瞬移后从旧位置补间；Kinematic 和 Static 不参与该插值缓存。

当前 `MeshComponent`、相机和灯光都会直接读取 GameObject 变换，因此采用“渲染阶段临时覆盖”方案：

```text
Scene::Update
  -> 固定步模拟
  -> Dynamic 当前物理姿态写回 GameObject（simulation transform）
  -> 普通 Update 读取最新模拟姿态

Scene::Render
  -> RigidBodyComponent 保存当前 GameObject 世界姿态
  -> 临时写入 interpolated presentation transform
  -> 收集 CameraData / LightingData 并提交 RenderQueue
  -> 恢复保存的 simulation transform
```

恢复由 Render 内的作用域 guard 保证，包括 `BeginView()` 失败提前返回的路径。插值姿态不会写回 Bullet，也不会成为下一次模拟或普通 `OnUpdate()` 的输入。该方案无需立刻修改 GameObject 和 MeshComponent，但仍是过渡设计：以后若渲染快照、并行渲染或复杂动态父子层级成为真实需求，应把 simulation transform 与 presentation transform 正式分离。

标准 accumulator 插值显示的是 previous/current 之间的姿态，因此会引入不超过一个 fixed step 的视觉延迟。这是获得稳定平滑结果的预期代价，而不是额外模拟延迟。

当前仍要求 Dynamic RigidBody 位于根节点，或其父节点是静止、单位缩放的组织节点。多个互为父子的 Dynamic Body 暂不作为受支持用法，因为临时世界姿态覆盖的先后顺序会使层级语义复杂化。

## 坐标、单位与形状尺寸

引擎应统一采用：

- 长度：米；
- 时间：秒；
- 质量：千克；
- 角度 API：引擎现有公开旋转接口若用度，要在物理边界明确转换；角速度建议统一使用弧度/秒；
- 世界方向：当前引擎 Y-up，前方为 -Z；Bullet 侧转换函数集中处理，不在组件中散落。

建议在 `BulletConversions.h` 中提供纯转换函数：

```cpp
btVector3 ToBullet(const glm::vec3& value);
glm::vec3 ToGlm(const btVector3& value);
btQuaternion ToBullet(const glm::quat& value);
glm::quat ToGlm(const btQuaternion& value);
btTransform ToBulletTransform(const glm::vec3&, const glm::quat&);
```

GameObject scale 不应直接写入 `btTransform`；它属于 shape 尺寸或 `localScaling`。非均匀缩放对 Sphere/Capsule 等形状语义不自然，第一版应拒绝或明确取最大轴，不能静默产生意外结果。

## Collision Shape 与资产

Box/Sphere/Capsule 是小型值描述，可以直接创建。ConvexHull/TriangleMesh 需要较多顶点数据，长期应作为可共享 PhysicsShape 资源，而不是每个实例从 Render Mesh 重建：

```text
Model/Mesh Asset
  -> 离线或加载期生成 Collider 数据
  -> PhysicsShape（只读、共享）
  -> 每个 Body 实例引用 shape
```

不要让 Physics 层依赖 OpenGL Mesh 或 VAO/VBO。渲染 Mesh 可以作为生成碰撞资产的输入，但 PhysicsShape 最终只保存 CPU 几何或后端形状。`btBvhTriangleMeshShape` 所引用的 triangle mesh 数据必须与 shape 同寿命，不能传入临时 vector 后释放。

多个刚体引用同一不可变 shape 时可以缓存；带不同 local scale 的实例要谨慎，因为直接修改共享 Bullet shape 的 scaling 会影响所有实例。可以把 scale 纳入缓存 key，或第一版为不同 scale 创建独立 shape。

当前实现尚未加入物理形状缓存。每个 Body 独立拥有其 Bullet shape；TriangleMesh 的 backing mesh 与 shape 一起由对应 BodySlot 持有，以保证生命周期正确。先保留这一实现，等模型实例数量和加载成本证明缓存有实际收益后，再设计不可变 PhysicsShape 资源及其缓存 key。

## 碰撞层、Trigger 与过滤

建议公开稳定的 layer/mask，而不是 Bullet flag：

```cpp
using CollisionLayer = uint16_t;

struct CollisionFilter {
    CollisionLayer belongsTo = 1;
    CollisionLayer collidesWith = 0xFFFF;
};
```

Bullet broadphase group/mask 是短整型语义，第一版直接采用 16 位能避免虚假的 32 位能力。是否允许碰撞的基本规则为双方掩码互相包含；若以后需要“只查询不碰撞”等规则，再增加单独 QueryMask。

Trigger/Sensor 仍参与 broadphase 和 overlap 检测，但设置 no-contact-response，不产生物理解算。Trigger 事件与普通碰撞事件共享配对生命周期，但事件类型不同。

## 碰撞事件

不要在 Bullet 内部回调或 manifold 遍历期间直接调用 GameObject 脚本。组件可能在回调中销毁对象、移除刚体或 Clear Scene，这会破坏 Bullet 当前迭代。

当前实现会在每个 fixed step 的 `stepSimulation()` 后遍历 dispatcher manifold，得到本步接触 pair，并与上一步集合比较：

```text
currentPairs - previousPairs -> Enter
currentPairs ∩ previousPairs -> Stay
previousPairs - currentPairs -> Exit
```

Pair key 使用带 generation 的两个 `PhysicsBodyHandle`，其相等和哈希语义不依赖可能复用的 Bullet 裸地址。只有 `distance <= 0` 的实际接触点会进入当前集合，避免把 Bullet manifold 中仍被缓存但已经分离的点误报为碰撞。Trigger 状态与 pair 一起保存，因此 Exit 仍能正确区分 Collision 和 Trigger。

事件使用值类型写入 PhysicsWorld 队列：

```cpp
struct ContactPoint {
    glm::vec3 position;
    glm::vec3 normal;
    float penetrationDepth = 0.0f;
    float appliedImpulse = 0.0f;
};

struct CollisionEvent {
    PhysicsBodyHandle self;
    PhysicsBodyHandle other;
    PhysicsEventPhase phase; // Enter / Stay / Exit
    bool trigger = false;
    std::vector<ContactPoint> contacts;
};
```

`Scene::Update()` 开始时清除上一帧事件；本帧所有 fixed step 产生的事件会累积，并在 `Update()` 返回后继续可由 `PhysicsWorld::GetCollisionEvents()` 查询。这样事件不会在生成后立即丢失，也不会把 Bullet manifold 指针暴露出去。一帧包含多个 fixed step 时，同一 pair 可能依次产生 Enter 和 Stay，这是固定步事件的真实顺序，调用方若只关心帧级状态需要自行归并。

当前尚未实现 Scene 到 Component 的自动回调分发，也未增加 `OnCollisionEnter/Stay/Exit` 与 `OnTriggerEnter/Stay/Exit`。在回调 API 确定前，游戏代码先轮询只读事件队列。未来分发前必须再次通过 generation 和 Scene 刚体注册表解析有效组件；对象销毁时是否向另一方补发 Exit 也应在加入回调时形成明确契约。

## 查询 API

查询应返回引擎值类型和安全句柄：

```cpp
struct RaycastDesc {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    float maxDistance = 1000.0f;
    uint16_t collisionMask = 0xFFFF;
    bool includeTriggers = false;
};

struct RaycastHit {
    PhysicsBodyHandle body;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
    float fraction = 0.0f;
};
```

当前已经实现 `RaycastClosest`。direction 在边界处归一化；零向量、非正或非有限 maxDistance、非有限 origin/direction 会直接失败。`collisionMask` 控制查询过滤，`includeTriggers` 控制是否命中 Trigger，结果通过带 generation 的 Body Handle 返回。

`RaycastAll`、`OverlapSphere` 尚未实现；Sweep/ShapeCast 留到角色控制器阶段再加入。查询期间不能修改 World。

## Bullet userPointer

Bullet 允许给 collision object 设置 user pointer/index。不要直接存长期有效的 `GameObject*` 并假设它不会悬空。优先存一个由 PhysicsWorld 管理的 Body record 指针或稳定 ID，record 再持有 `PhysicsBodyHandle` 和短期观察引用；注销 Body 时先从 World 移除，再清除 user pointer，最后回收 record。

这也能让 PhysicsWorld 在事件和 Raycast 中返回 Handle，而不把 Bullet 类型或 GameObject 生命周期泄漏到查询实现。

## Debug Draw

Bullet 的 `btIDebugDraw` 可以接入引擎，但实现不应直接调用 OpenGL。Debug drawer 只收集线段、颜色和文本诊断，随后通过 RenderQueue 的 DebugLine pass 绘制。当前尚无 DebugLine 渲染路径时，可以先实现开关和 CPU 线段收集，或暂缓 Debug Draw，不要在 Physics 模块中引入 OpenGL 依赖。

## CMake 与依赖边界

当前 CMake 已关闭 Bullet demos、tests、PyBullet、Bullet3 和 extras，并链接 `BulletDynamics`、`BulletCollision`、`LinearMath`，适合作为第一版起点。

后续建议：

- Bullet include path 尽量作为 Physics 后端的 `PRIVATE` 依赖；只有公开头确实包含 Bullet 类型时才需要 PUBLIC，而本设计不需要公开 Bullet 类型；
- `BulletDynamics/BulletCollision/LinearMath` 对静态 engine 的最终链接传播需要由 CMake target 关系保证，不要改回手写 `.a` 路径；
- Windows 下确认 Bullet 与 Engine 使用相同编译器、运行库、Debug/Release 配置和标量精度；
- 当前固定 `BUILD_SHARED_LIBS OFF ... FORCE` 会影响同一 CMake 构建中的其他第三方库，长期可以在引入 Bullet 前保存原值并在 `add_subdirectory` 后恢复，或把 Bullet 构建放到更独立的作用域；
- Bullet 源码现在位于 `engine/include/bullet3`，可运行但语义上不是普通 header-only include。以后可以移动到 `third_party/bullet3`，让 `include/` 只保存真正公开或头文件依赖。

## 与现有 Scene 延迟命令的关系

Scene/GameObject 的结构修改已经是延迟执行的，物理也需要类似安全边界，但不要复用同一个 `SceneCommand` variant 塞入 Bullet 指针。PhysicsWorld 保持自己的强类型命令：

```text
PhysicsCommand
├─ CreateBodyCommand
├─ DestroyBodyCommand
├─ ReplaceShapeCommand
├─ SetFilterCommand
└─ TeleportCommand（若 Step 中请求）
```

Scene Flush 决定对象/组件所有权，Physics Flush 决定 Bullet World 注册状态。两者的顺序必须固定：对象创建和组件 attach 完成后才能创建 Body；Body 从 World 注销完成后才能释放组件后端记录。若第一版还没有统一 OnAttach/OnDetach，可让 RigidBodyComponent 析构调用 World 的安全 `DestroyBody()`，该接口在非 Step 阶段立即执行、Step/事件派发期间则 staged。

## 建议实施顺序

### 当前已完成的基础

当前代码已经建立 `PhysicsWorld -> Impl -> BulletPhysicsWorld` 边界，完成 Bullet collision configuration、dispatcher、broadphase、solver、discrete dynamics world 和重力初始化；Scene 默认持有独立 PhysicsWorld，并在外层 accumulator 中调用固定步长 `stepSimulation(fixedTimeStep, 0, fixedTimeStep)`。GameObject/Component 也已经增加 FixedUpdate 树遍历。

基础检查后补充了以下约束：`PhysicsWorldDesc` 会拒绝非有限重力、非正 fixed step、零 substep 预算和小于 fixed step 的累计上限；Scene 会拒绝负数/非有限 delta，累计总量被限制在 `maxAccumulatedTime`，耗尽 substep 预算后丢弃逾期的完整步，避免跨帧形成无限追赶；替换 PhysicsWorld 会验证配置、禁止在 Update 中执行并重置 accumulator；重力修改会同步到公开 Desc。PhysicsWorld 地址固定，不支持复制或移动，为以后组件保存 World 观察引用提供稳定语义。Bullet include 与库链接也已经收回 engine 的 PRIVATE 边界。

当前已经实现基础 CollisionShape 描述、带 generation 的 Body Handle、Static/Kinematic/Dynamic RigidBody、Compound Shape、力/冲量/速度、Teleport、启用状态，以及固定步前后的 Kinematic Push 和 Dynamic Pull。GameObject/Component 的 Scene 生命周期也已补齐：挂接后调用 `OnAttach`，组件移除、对象销毁和 Scene Clear 前调用 `OnDetach`；RigidBody 在这两个回调中对称创建和销毁 Bullet Body。Scene 维护非拥有的刚体注册表，用于集中执行 Kinematic Push、Dynamic Pull 和渲染插值；创建 Body 失败的组件不会进入注册表。

实现检查进一步明确：Compound 的 child shape 和 TriangleMesh 的 backing mesh 必须由 BodySlot 一并拥有；单 Collider 的 local offset 也必须通过 Compound 表达；混合 Trigger/非 Trigger Collider 在当前“一个 Bullet Body”模型下会被拒绝。PhysicsWorld 只能在空 Scene 中替换，避免现有 RigidBodyComponent 保存的 World 观察指针失效。

此外已经实现 `RaycastClosest`、Trigger/Collision 的 Enter/Stay/Exit 值事件队列，以及 Dynamic Body 的 presentation interpolation。渲染插值只在 Render 提交期间临时覆盖 GameObject，并在所有返回路径恢复模拟姿态；Teleport 会重置插值历史，避免视觉拖影。

1. **已完成**：建立 `PhysicsTypes.h`、GLM/Bullet 转换函数和 `PhysicsWorld::Impl`，创建/销毁 Bullet World 并设置重力。
2. **已完成**：实现 Static Body，并保证 Scene Clear、组件删除和 World 析构时注销 Body。
3. **已完成**：增加 Dynamic Body、质量/惯性计算、固定时间步和 Physics -> GameObject 世界/局部变换同步。
4. **已完成**：增加 Kinematic Body、Teleport、力/冲量、速度和激活接口，明确三种 Body 的 transform authority。
5. **基本完成**：增加 Box/Sphere/Capsule/ConvexHull/Compound 和 Static TriangleMesh；共享 PhysicsShape 与形状缓存明确暂缓。
6. **部分完成**：加入 collision layer/mask、Trigger 和 Enter/Stay/Exit 事件队列；Component 自动回调分发尚未实现。
7. **部分完成**：实现 `RaycastClosest` 并用 Body Handle 返回命中对象；`RaycastAll`、Overlap 和 Sweep 尚未实现。
8. **未实现**：Debug Draw 和基础统计，包括 body 数量、active body、substep 数、contact 数与超预算警告。
9. **已完成基础版本**：presentation interpolation；后续复杂动态父子层级或并行渲染出现时，再升级为独立 presentation transform/渲染快照。

## 最小验证清单

- 静态地面和动态 Box 在不同帧率下落地位置近似一致；
- 30/60/144 FPS 下，相同固定步数后的物理结果一致到可接受误差；
- 大帧卡顿不会无限执行子步；
- Dynamic Body 不会被普通 OnUpdate 每帧写回旧 Scene transform；
- Kinematic Body 能推动 Dynamic Body，但不被反向覆盖；
- 删除 GameObject、删除 RigidBodyComponent 和 Scene::Clear 后 World 中不残留 Body；
- Trigger 不产生碰撞响应，但 Enter/Stay/Exit 顺序正确；
- 加入组件碰撞回调后，在回调中销毁对象不会使 manifold 遍历或事件派发崩溃；
- layer/mask 双向过滤符合预期；
- Raycast 可选择忽略 Trigger，并不会返回已销毁 Body；
- Dynamic 子节点在允许的父节点约束下能正确把世界变换转换为局部变换；
- 多个 Scene 的重力、Body 和事件完全隔离。

## 当前最重要的设计结论

第一，PhysicsWorld 属于 Scene，不属于 Window、RenderDevice 或全局 Engine。第二，物理使用固定步长并拥有独立 FixedUpdate 阶段，不能直接依赖可变帧 delta。第三，Static/Kinematic/Dynamic 必须有明确且单向的 Transform 控制权。第四，Bullet 类型只留在 Physics 实现边界，公开组件使用 Desc、值类型和 Handle。第五，Body 的注册、销毁和事件派发必须发生在安全点，不能在 Bullet Step 或 manifold 遍历中直接改变 World。
