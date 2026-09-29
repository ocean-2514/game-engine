# 动画与骨骼蒙皮架构

## 两类需要区分的动画

当前 `AnimationComponent` 实现的是节点变换动画：动画轨道按时间采样局部位置、旋转和缩放，再写入对应的 `GameObject`。这适用于门、摄像机、刚体部件等节点动画，也能驱动实例化出来的模型节点层级。

骨骼动画在此基础上还需要蒙皮。改变骨骼节点的局部变换只能得到骨骼当前姿态；网格顶点不会自动跟随骨骼。每个顶点还需要骨骼索引及权重，并利用当前骨骼矩阵计算最终位置和法线。

因此应当把系统分为三个部分：

```text
Model（共享资产）
  ├─ 节点/骨架的静态拓扑和绑定姿势
  ├─ Mesh 及每顶点蒙皮信息
  └─ AnimationClip 列表

AnimationComponent（每个模型实例的运行状态）
  ├─ 当前 Clip、时间、速度、循环状态
  ├─ 对 Clip 采样并生成当前局部/全局姿态
  └─ 生成该实例的 skinning palette

渲染路径
  ├─ MeshComponent/SkinnedMeshComponent 提交 Mesh、Material 和 palette
  └─ RenderQueue 把 palette 提交给支持蒙皮的 Shader
```

资产数据可以被多个实例共享，但动画播放时间和骨骼姿态必须属于实例。不要把当前姿态写回共享的 `Model` 或共享的 `Mesh`。

## 建议的数据组织

`AnimationClip` 不应定义在组件头文件中。它是可共享、可缓存的资产数据，宜放入独立的动画数据头文件，并由 `Model` 保存 `shared_ptr<const AnimationClip>` 或值类型数组。Clip 本身保持不可变，循环、播放速度等属于播放器状态，不属于 Clip；导入文件中的默认行为可作为独立元数据。

一个基础骨架至少需要节点层级与 Bone 绑定数据。当前实现直接复用
`ModelNode` 作为骨架节点，避免为同一层级维护两份结构；概念上等价于：

```cpp
// ModelNode already supplies name, bind local transform and children.

struct Bone {
    uint32_t nodeIndex;
    glm::mat4 inverseBindMatrix;
};
```

动画轨道最好以稳定的 `nodeIndex` 为目标，而不是在运行时反复按名称寻找 `GameObject`。Assimp 导入阶段可以建立 `aiNode name -> ModelNode index` 映射，再把 `aiNodeAnim::mNodeName` 转成索引。名称只用于导入、调试和对外查询；重复名称必须在导入时检测，必要时使用完整节点路径。

每个可蒙皮顶点通常保存固定数量的影响，例如：

```text
location 0: position   vec3
location 1: normal     vec3
location 2: uv0        vec2
location 3: boneIds    ivec4
location 4: weights    vec4
```

导入 `aiBone::mWeights` 时，对每个顶点累积影响，按权重选取最大的四项并重新归一化。无影响顶点需要明确的退化策略，例如权重 `(1, 0, 0, 0)` 指向恒等骨骼。超过 Shader 能力的骨骼数或无法解析的骨骼名称应产生明确错误，不能静默截断。

如果不同子网格使用同一骨架但只引用其中一部分骨骼，可先让所有网格使用模型级统一骨骼索引。以后需要优化时，再为每个 `ModelMesh` 增加局部 palette 到模型骨骼索引的重映射。

## Assimp 动画转换

Assimp 类型只存在于 `ModelAssetLoader.cpp` 的导入边界内，运行时数据不保存 `aiScene`、`aiAnimation` 或 `aiBone` 指针。Importer 销毁后这些指针都会失效。

导入每个 `aiAnimation` 时：

1. `ticksPerSecond` 若大于零，所有关键帧时间和 duration 都除以它，统一转换为秒。
2. `ticksPerSecond == 0` 时使用一个明确且集中定义的回退值，并记录该策略；不要让 tick 值直接进入运行时组件。
3. 将 `aiNodeAnim` 的位置、旋转和缩放关键帧转换为引擎类型，并将目标名称解析为稳定节点索引。
4. 对缺少某一通道的节点，使用绑定姿势中对应的 T/R/S 分量，而不是默认零位置、单位旋转或单位缩放覆盖整个绑定姿势。
5. 关键帧应保证按时间排序；采样结果中的四元数应归一化。

对于 Assimp，`aiBone::mOffsetMatrix` 表示从网格/模型绑定空间到骨骼绑定空间的逆绑定矩阵。基础蒙皮矩阵可按以下思路计算：

```text
palette[bone] = inverse(modelRootGlobalBind)
              * currentBoneGlobal
              * inverseBindMatrix
```

具体是否需要根节点修正取决于导入后 `ModelNode` 与 Mesh 使用的坐标空间，必须用一个只有单骨骼平移或旋转的测试资产验证。矩阵顺序不要仅凭示例代码照搬。

## AnimationComponent 的职责

建议让 `AnimationComponent` 逐步成为播放器/姿态求值器：

- 持有注册的只读 Clip，以及当前 Clip 的安全引用；
- 保存时间、速度、循环、暂停状态；
- 采样轨道，先得到局部姿态，再沿父子顺序计算全局姿态；
- 生成并拥有该模型实例的骨骼 palette；
- 提供只读 palette 给渲染组件，而不直接操作 `Mesh` 的共享顶点数据。

当前通过名称查找并直接修改子 `GameObject` 的方式可以保留为节点动画的第一阶段，但需要修正：

- `SetPlaying(bool)` 当前无论参数为何都写入 `false`；
- `Play()` 在当前 Clip 为空时解引用空指针；
- 切换 Clip 后没有重建 bindings；
- `Register()` 没有检查空指针；
- duration 为零时循环播放会执行对零 `fmod`；
- 非循环动画结束时应停在结尾姿态，而不是把时间重置到零；
- `SetTime()` 应按当前 Clip 的 duration clamp/wrap，并立即或在下一次更新时重新采样；
- bindings 保存裸 `GameObject*`，对象销毁或层级变化后可能失效；播放期间至少要检查存活性并在结构变化后重建；
- 重名节点会错误绑定，线性插值查找也会随关键帧数量增长。

组件当前的 `AnimationClip*` 还可能指向未注册或已经销毁的对象。更安全的选择是让当前 Clip 保存 `shared_ptr<const AnimationClip>`，或保存注册表中的稳定句柄/索引。

## 与 MeshComponent 和 RenderCommand 的边界

普通 `MeshComponent` 不应直接依赖 `AnimationComponent`。普通网格没有 palette，现有提交路径保持不变。对于蒙皮网格有两种渐进方案：

1. 短期可以增加 `SkinnedMeshComponent`，在模型实例化时与 `AnimationComponent` 共享一个实例级 `SkeletonPose`。它提交带 palette 的命令。
2. 如果暂时不想增加组件类型，可给 `MeshComponent` 增加可选的 `SkeletonPose` 引用；但这会让普通组件承担蒙皮分支，长期可读性较差。

`RenderCommand` 可以增加可选的 `SkinningData`/palette 句柄，而不保存 `AnimationComponent*`。渲染层只关心本次绘制所需矩阵，不应知道动画如何播放和采样。对普通命令该字段为空；对蒙皮命令，RenderQueue 在绘制前上传 palette。

OpenGL 3.3 的第一版可以用固定上限的 `uniform mat4 uBones[MAX_BONES]`。这容易实现和调试，但需要与 Shader 上限保持一致。之后再将 palette 改为 UBO、纹理缓冲或 SSBO。这个变化应封装在渲染后端或 GPU 参数资源内，不改变 AnimationComponent 的采样职责。

GPU 蒙皮应作为默认实现。CPU 蒙皮适合参考实现、调试或极少量网格，但它每帧需要改写并上传顶点，而且共享 Mesh 不能直接被多个不同动画实例修改。无论 CPU 还是 GPU，AnimationComponent 都应输出同一种逻辑 palette。

蒙皮 Shader 必须同时处理位置和法线。基础形式为：

```glsl
mat4 skin = weights.x * uBones[boneIds.x]
          + weights.y * uBones[boneIds.y]
          + weights.z * uBones[boneIds.z]
          + weights.w * uBones[boneIds.w];

vec4 localPosition = skin * vec4(vPos, 1.0);
vec3 localNormal = mat3(skin) * vNormal;
```

随后再应用对象的 `uModel` 和现有 normal matrix。存在非均匀骨骼缩放时需要更严格的法线处理；第一版可以声明只支持刚性旋转、平移和均匀缩放。

## 推荐的渐进实施顺序

1. 先修正现有 AnimationComponent 的播放状态、时间边界、空指针与 binding 重建问题，并把 Clip 数据移出组件头文件。
2. 完成 Assimp 节点动画导入，以秒为单位驱动现有 GameObject 层级，验证轨道和坐标转换。
3. 在 Model 中加入骨架、逆绑定矩阵和每顶点四骨骼影响；导入阶段完成名称到索引的解析。
4. 让 AnimationComponent 输出实例级局部姿态、全局姿态和 palette，不再依赖名称驱动骨骼 GameObject。
5. 增加可选蒙皮绘制数据和蒙皮 Shader，先采用固定上限 uniform 数组完成 GPU 蒙皮。
6. 最后加入动画混合、淡入淡出、事件和 root motion，再根据数量与性能迁移 palette 上传方式。

## 当前已落地的最小链路

当前实现采用以下所有权关系：`Model` 是共享且只读的资产，保存 `ModelNode`、Bone、AnimationClip、Mesh 和 Material；`AnimationComponent` 持有 `shared_ptr<const Model>`，并独占该实例的播放状态，同时通过 `shared_ptr<SkeletonPose>` 向一个或多个 `SkinnedMeshComponent` 提供当前 palette。`SkinnedMeshComponent` 不访问播放器，也不修改 Model，只把 Pose 作为可选蒙皮数据放进 `RenderCommand`。

这里没有再维护一套与 `ModelNode` 重复的 `SkeletonNode`。动画轨道和 Bone 都以 `ModelNode` 索引作为稳定目标；Bone 只是 ModelNode 子集上的额外 inverse-bind 数据。这对于当前单一模型层级足够，未来只有在支持独立 Skeleton 资产和动画重定向时，才值得拆出单独 Skeleton。

姿态计算从模型根节点向子节点递归：

```text
global(node) = global(parent) * animatedLocal(node)
skin(bone)   = inverse(rootBind) * global(boneNode) * inverseBind(bone)
```

这是有父子依赖时的正确方向。父节点的全局矩阵必须先得到，因此不应从叶子向根计算；只有在计算包围盒等需要汇总子节点结果的任务中才需要向上遍历。

模型实例化时，Scene 在实例根创建一个 AnimationComponent，并把同一个 SkeletonPose 交给所有蒙皮子网格。蒙皮命令使用模型实例根的世界矩阵作为 `uModel`，而不是网格所在骨骼节点的世界矩阵，否则节点动画会与 palette 重复应用。普通 MeshComponent 仍使用自身 GameObject 的世界矩阵。

当前顶点上传接口仍是 `vector<float>`，所以骨骼 ID 暂时编码为可精确表示的小整数 float，在 Shader 中转成 `ivec4`。这是兼容现有 Mesh API 的过渡方案；以后把 Mesh 创建接口改为原始字节数据或结构化 VertexBuffer 后，应恢复真正的整型顶点属性。

`ModelLoadOptions` 分别提供 `importSkeletons` 和 `importAnimations`。节点动画并不一定包含蒙皮骨架，因此两者不能共用同一个开关；两项也都进入 Model 缓存键。

## 为动画事件预留的空间

基础 Clip 可预留按时间排序的事件标记：事件只包含时间、名称和轻量 payload，不存 `std::function`。组件在时间从旧值推进到新值时收集跨过的事件，再交给上层回调或消息系统。循环时需要分别处理 `[oldTime, duration]` 和 `[0, newTime]`，大步长跨越多个循环时也不能漏发。

在基础播放和蒙皮稳定以前不需要实现事件，但时间推进逻辑应集中在一个位置，并保留“上一采样时间”，避免以后为事件系统重写播放器。

## 动画切换与混合

### 先统一 Pose 表示

动画混合不应直接混合最终的 `skinMatrices`。矩阵线性插值会破坏旋转正交性，也难以正确处理缩放。各 Clip 应先采样成模型节点的局部 TRS Pose，再在局部空间逐节点混合，最后仅进行一次全局矩阵传播和 skinning palette 生成：

```text
Clip A ──采样──> LocalPose A ─┐
                              ├─ TRS Blend ─> FinalLocalPose
Clip B ──采样──> LocalPose B ─┘                  │
                                                  ▼
                            root-to-leaf global transforms
                                                  │
                                                  ▼
                                        skinning palette
```

建议增加值类型：

```cpp
struct LocalTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct AnimationPose {
    // 与 ModelNode 索引一一对应。
    std::vector<LocalTransform> localTransforms;
};
```

平移和缩放用 `mix`，旋转用归一化后的 `slerp`；混合前确保两个 Pose 都完整。Clip 没有某个节点或某个通道时，用 bind pose 对应值补齐，而不是把“缺少通道”继续带入混合阶段。

这意味着当前 `SampleLocalTransform()` 应逐步拆成“采样完整 LocalPose”和“由 FinalLocalPose 生成 palette”两步。GameObject 节点变换也应在最终混合完成后统一写入一次，而不是每个输入 Clip 分别写入。

### 第一阶段：只实现 CrossFade

当前最值得先实现的是两个 Clip 间的交叉淡入，不需要立即引入完整状态机。可以先加入两个轻量运行时结构：

```cpp
struct AnimationPlayback {
    std::shared_ptr<const AnimationClip> clip;
    float time = 0.0f;
    float speed = 1.0f;
    bool looping = true;
};

struct AnimationTransition {
    AnimationPose sourcePose;
    AnimationPlayback destination;
    float elapsed = 0.0f;
    float duration = 0.2f;
};
```

`AnimationComponent` 暂时保留一个当前 Playback 和一个可选 Transition，并提供：

```cpp
bool Play(std::string_view clipName, bool looping = true);
bool CrossFade(std::string_view clipName,
               float duration,
               bool looping = true,
               float normalizedStartTime = 0.0f);
```

启动 CrossFade 时，推荐捕获“当前最终输出 Pose”作为 `sourcePose`，而不是只保存旧 Clip 和旧时间。这样在上一次过渡尚未结束时再次切换，也能从当前视觉姿态平滑过渡，不会跳回旧 Clip 的单独采样结果。

每帧流程为：

1. 推进 destination 的时间；
2. 采样 destination Pose；
3. 计算 `weight = clamp(elapsed / duration, 0, 1)`；
4. 将捕获的 sourcePose 与 destination Pose 混合；
5. weight 到达 1 后，把 destination 提升为当前 Playback 并结束 Transition；
6. 从最终 LocalPose 计算节点全局矩阵及 skinning palette。

`duration <= 0` 应等价于立即切换。第一版使用线性权重即可，之后可以给 Transition 增加 ease-in/out 曲线。Idle/Walk/Run 等周期动作还可选择按 normalized time 同步相位，但不应成为所有 CrossFade 的默认行为，例如攻击动作通常应从零开始。

### AnimationComponent 与控制器的边界

`AnimationComponent` 应当是 Animator 的运行时执行器，负责：

- 保存参数和当前运行状态；
- 推进播放时间；
- 调用 Clip/Blend Tree 采样；
- 混合各层 Pose；
- 输出最终 `SkeletonPose`；
- 提供显式的 `Play()`、`CrossFade()` 等低层接口。

游戏逻辑不应直接操作 `SkinnedMeshComponent`，后者仍只消费最终 palette。状态机、Blend Tree 和动画层都应位于 Pose 生成阶段，不改变当前 `AnimationComponent -> SkeletonPose -> SkinnedMeshComponent` 的所有权关系。

### 第二阶段：状态机

当手工调用 CrossFade 已稳定后，再增加数据驱动的 `AnimatorController`。建议区分共享定义与实例状态：

```text
AnimatorController（共享资产）
  ├─ Parameter 定义
  ├─ State 定义
  └─ Transition/Condition 定义

AnimationComponent（每实例）
  ├─ Parameter 当前值
  ├─ 当前 State 和状态时间
  └─ 当前 Transition 运行进度
```

基础状态机需要的概念：

- `AnimatorParameter`：Float、Bool、Int、Trigger；
- `AnimationState`：引用一个 Clip 或 Motion、速度、是否循环；
- `AnimationTransition`：源状态、目标状态、混合时长、退出时间和 Conditions；
- `TransitionCondition`：参数、比较操作和值；
- `AnyState` 可以稍后增加，不应进入第一版。

Trigger 是被成功采用的 Transition 消费的一次性参数，而不是普通 Bool。一次更新中最多提交一次状态切换，避免多个 Transition 同时修改当前状态。Transition 选择顺序必须稳定，可以直接使用声明顺序和显式优先级。

Controller 不负责采样骨骼，也不保存每实例时间。它只是共享的决策配置；`AnimationComponent` 根据 Controller 选择 Motion，再复用 CrossFade 和 Pose 求值代码。

#### 第二阶段的实现范围

第一版状态机只解决以下问题：

- 定义若干以单个 AnimationClip 为 Motion 的状态；
- 通过 Float、Bool、Int、Trigger 参数决定是否切换；
- 支持固定 CrossFade 时长；
- 可选地等待当前状态播放到指定 normalized exit time；
- 多个模型实例共享同一个 Controller 定义，但分别保存自己的状态和参数值。

第一版明确不实现 AnyState、Sub-State Machine、Blend Tree、Animation Layer、动画事件和 Root Motion。这些能力不能反向污染基础状态机的数据结构；State 对 Motion 的引用第一版可以直接使用 `clipName`，以后再把它替换成 `MotionId` 或 `shared_ptr<const Motion>`。

#### 定义数据与实例数据必须分离

状态机需要严格区分共享定义和运行状态：

```text
AnimatorController（共享、只读）
  ├─ ParameterDefinition[]
  ├─ AnimationStateDefinition[]
  ├─ AnimatorTransitionDefinition[]
  └─ defaultStateIndex

AnimationComponent（每模型实例）
  ├─ shared_ptr<const AnimatorController>
  ├─ ParameterValue[]
  ├─ currentStateIndex
  ├─ pending/destinationStateIndex
  ├─ AnimationPlayback
  ├─ AnimationBlendTransition
  └─ 最终 AnimationPose / SkeletonPose
```

Controller 不应持有 `AnimationPlayback`、当前状态时间、当前参数值或 `AnimationPose`。否则两个使用同一 Controller 的角色会共享播放进度。AnimationComponent 已经拥有 Clip 播放、CrossFade 和 Pose 求值能力，因此状态机只是位于它上方的决策层，不能创建第二套播放器。

`Animation.h` 建议继续只保存 Clip、Pose、Playback 和底层 Blend Transition 等动画求值数据；状态机定义放在 `AnimatorController.h`，避免所有只使用 Clip 的代码都依赖状态机概念。

#### 建议的 Controller 定义

可以先使用稳定整数索引作为 ID，避免保存指向 `vector` 元素的指针：

```cpp
using AnimatorParameterId = uint32_t;
using AnimatorStateId = uint32_t;

inline constexpr uint32_t InvalidAnimatorId =
    std::numeric_limits<uint32_t>::max();

enum class AnimatorParameterType {
    Float,
    Bool,
    Int,
    Trigger
};

using AnimatorParameterValue =
    std::variant<float, bool, int32_t>;

struct AnimatorParameterDefinition {
    std::string name;
    AnimatorParameterType type = AnimatorParameterType::Float;
    AnimatorParameterValue defaultValue = 0.0f;
};

struct AnimationStateDefinition {
    std::string name;
    std::string clipName;
    float speed = 1.0f;
    bool looping = true;
};
```

当前草稿中的 `AnimationState::looping` 应为 `bool`，不是 `float`。状态名称和参数名称在同一 Controller 内必须唯一；`speed` 可以为负数，但零速度状态需要明确允许。第一版 State 通过 `clipName` 引用 AnimationComponent 已注册的 Clip，Controller 构建时无法验证 Clip，绑定到具体 AnimationComponent/Model 时再验证。

参数默认值必须与声明类型一致。Trigger 在存储上可以使用 bool，但语义不是普通 Bool：它被设置后保持 pending，直到某条实际采用的 Transition 消费它；未选中的 Transition 不能清除 Trigger。

#### Condition 的类型规则

不同参数类型只允许有限的比较操作：

```cpp
enum class AnimatorConditionOp {
    IsTrue,
    IsFalse,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    IsTriggered
};

struct AnimatorCondition {
    AnimatorParameterId parameterId = InvalidAnimatorId;
    AnimatorConditionOp op = AnimatorConditionOp::IsTrue;
    AnimatorParameterValue threshold = false;
};
```

约束如下：

| 参数类型 | 允许的操作 |
| --- | --- |
| Float | Greater、GreaterEqual、Less、LessEqual；Equal 不建议用于浮点数 |
| Int | Greater、GreaterEqual、Less、LessEqual、Equal、NotEqual |
| Bool | IsTrue、IsFalse |
| Trigger | IsTriggered |

一条 Transition 内的所有 Conditions 使用 AND；从同一 State 出发的多条 Transition 相互之间是 OR 候选。Controller 在添加或 `Validate()` 时应拒绝类型不匹配的 Condition，而不是在每帧更新时临时猜测类型。

#### Transition 定义

状态机 Transition 与当前用于 Pose 混合的 `AnimationBlendTransition` 不是同一层概念，应使用不同名称：

```cpp
struct AnimatorTransitionDefinition {
    AnimatorStateId sourceState = InvalidAnimatorId;
    AnimatorStateId destinationState = InvalidAnimatorId;

    float blendDuration = 0.2f;
    bool hasExitTime = false;
    float exitTimeNormalized = 1.0f;
    float destinationStartNormalized = 0.0f;

    std::vector<AnimatorCondition> conditions;
};
```

含义是：`AnimatorTransitionDefinition` 决定“何时从哪个 State 去哪里”，而 `AnimationBlendTransition` 保存这一次切换已经捕获的 sourcePose、目标 Playback 和 elapsed。Controller 定义不会随播放更新。

第一版可以用 Controller 中 Transition 的声明顺序作为稳定优先级：从头扫描，第一条满足的 Transition 获胜。不要遍历 `unordered_map` 选择 Transition，否则不同运行或平台上的行为可能不一致。以后若需要显式优先级，再增加整数 `priority`，同优先级仍保持声明顺序。

`blendDuration <= 0` 表示立即切换，并复用现有 `Play()` 语义。`destinationStartNormalized` clamp 到 `[0, 1]`，实际目标时间为：

```text
destinationTime = destinationStartNormalized * destinationClip.duration
```

#### Exit Time 语义

`hasExitTime == false` 时，只要 Conditions 满足就可以切换。`hasExitTime == true` 时，当前状态还必须到达指定 normalized time：

```cpp
normalizedTime = clip.duration > 0
    ? playback.time / clip.duration
    : 1.0f;
```

第一版将 `exitTimeNormalized` 限定在 `[0, 1]`。循环 Clip 每一轮到达该比例后都有一次切换机会；非循环 Clip 到达结尾后保持为 1。为了以后准确判断“这一帧是否跨过 exit time”和处理大 deltaTime，AnimationPlayback 应逐步保留 `previousTime` 或累计循环计数。第一版如果仅使用 `normalizedTime >= exitTimeNormalized`，要明确它表示从 exit time 到本轮结束都是有效窗口。

只有 Exit Time、所有 Conditions 和目标状态有效性同时满足时，Transition 才可采用。没有 Conditions 但有 Exit Time 的 Transition 表示动画播放到该位置后自动切换；两者都没有的 Transition 会在进入 State 后立即发生，应允许但在 Validate 中给出警告，因为它很容易造成无意的状态跳转或状态环。

#### AnimatorController 的最小接口

Controller 可以先作为纯 C++ 构建的共享对象：

```cpp
class AnimatorController {
public:
    AnimatorParameterId AddFloat(std::string name,
        float defaultValue = 0.0f);
    AnimatorParameterId AddBool(std::string name,
        bool defaultValue = false);
    AnimatorParameterId AddInt(std::string name,
        int32_t defaultValue = 0);
    AnimatorParameterId AddTrigger(std::string name);

    AnimatorStateId AddState(AnimationStateDefinition state);
    bool AddTransition(AnimatorTransitionDefinition transition);
    bool SetDefaultState(AnimatorStateId state);

    AnimatorParameterId FindParameter(std::string_view name) const;
    AnimatorStateId FindState(std::string_view name) const;

    const AnimatorParameterDefinition* GetParameter(
        AnimatorParameterId id) const;
    const AnimationStateDefinition* GetState(
        AnimatorStateId id) const;
    const std::vector<AnimatorTransitionDefinition>&
        GetTransitionsFrom(AnimatorStateId state) const;

    bool Validate(std::string* error = nullptr) const;
};
```

具体返回类型可以按现有项目风格调整，但要保持几个原则：失败不能返回一个看似有效的索引；外部不能修改 Controller 内部数组；构建完成并交给 AnimationComponent 后把 Controller 当作不可变资产。当前空的显式构造函数和析构函数没有必要，使用默认生成即可。

`GetTransitionsFrom()` 若直接返回每个状态自己的 Transition 数组，可以将出边存在 State 定义中；若所有 Transition 集中存储，则 Controller 可维护 `state -> transition indices`。第一版状态少时直接扫描总数组也可以，但 Transition 选择顺序必须稳定。

#### AnimationComponent 增加的运行时职责

AnimationComponent 可以增加 Controller 绑定和参数 API：

```cpp
bool SetController(std::shared_ptr<const AnimatorController> controller);
const std::shared_ptr<const AnimatorController>& GetController() const;

bool SetFloat(std::string_view name, float value);
bool SetBool(std::string_view name, bool value);
bool SetInt(std::string_view name, int32_t value);
bool SetTrigger(std::string_view name);
bool ResetTrigger(std::string_view name);

AnimatorStateId GetCurrentState() const;
bool IsInTransition() const;
```

内部至少保存：

```cpp
std::shared_ptr<const AnimatorController> m_controller;
std::vector<AnimatorParameterValue> m_parameterValues;
AnimatorStateId m_currentState = InvalidAnimatorId;
AnimatorStateId m_destinationState = InvalidAnimatorId;
```

`SetController()` 应完成一次完整绑定：

1. 检查 Controller 本身 Validate 通过；
2. 验证每个 State 的 `clipName` 都能在 `m_clips` 中找到；
3. 按 ParameterDefinition 初始化每实例参数值；
4. 设置 currentState 为 defaultState；
5. 取消旧的 AnimationBlendTransition；
6. 通过内部的立即播放逻辑进入默认 State；
7. 任一步失败都不留下半初始化 Controller 状态。

状态的 speed 要写入目标 `AnimationPlayback::speed`。当前 `Play()` 和 `CrossFade()` 只接受 clipName、duration 和 looping，因此实现状态机前最好给内部切换函数增加 destination speed 和 normalized start time；不一定需要把所有参数暴露成公共 API。

#### 每帧状态机更新顺序

建议把当前 `OnUpdate()` 拆成明确的顺序，但仍可保留在同一个 AnimationComponent 类内：

```text
1. 检查暂停和 deltaTime
2. 推进当前 Playback 或 AnimationBlendTransition
3. 计算当前状态 normalized time
4. 若当前不在过渡中，按声明顺序检查当前 State 的出边
5. 采用至多一条 Transition
6. 使用现有 CrossFade 初始化底层 Pose 过渡
7. 采样本帧最终 LocalPose
8. 更新 GameObject bindings
9. 生成 SkeletonPose
10. 消费被采用 Transition 使用的 Trigger
```

第一版建议“过渡期间不再检查新 Transition”。这会暂时失去过渡打断能力，但语义最简单、最容易验证。手工 `CrossFade()` 已经支持从当前混合 Pose 中断，因此以后只需为 Transition 定义增加 interruption policy，再开放目标状态或源状态的候选检查，不需要重写 Pose 混合。

需要注意状态 ID 与底层 playback 的提交时机：开始 CrossFade 时，`m_currentState` 仍表示源 State，`m_destinationState` 表示目标；CrossFade 完成时才令：

```cpp
m_currentState = m_destinationState;
m_destinationState = InvalidAnimatorId;
```

这样 `GetCurrentState()`、事件归属和以后过渡打断规则都有明确含义。

#### Transition 选择伪代码

```cpp
bool AnimationComponent::TryStartStateTransition() {
    if (!m_controller || m_transition.active) return false;

    const auto& candidates =
        m_controller->GetTransitionsFrom(m_currentState);

    for (const auto& transition : candidates) {
        if (!HasReachedExitTime(transition)) continue;
        if (!AreConditionsMet(transition.conditions)) continue;
        if (!CanEnterState(transition.destinationState)) continue;

        StartStateTransition(transition);
        ConsumeTriggersUsedBy(transition);
        return true;
    }
    return false;
}
```

所有检查在真正修改状态前完成。若目标 Clip 不存在、目标 State 无效或 CrossFade 初始化失败，不能消费 Trigger，也不能改变 current/destination state。

#### 参数查找和性能

公共接口使用字符串便于游戏代码和未来 JSON 配置：

```cpp
animator->SetFloat("speed", velocityLength);
animator->SetBool("grounded", isGrounded);
animator->SetTrigger("attack");
```

但每帧不应反复在多个字符串 map 间查找。Controller 可以在构建时将名称映射到 ParameterId，Condition 只保存 ParameterId；AnimationComponent 参数值使用与定义数组相同索引的 `vector`。对性能敏感的游戏逻辑以后可以缓存 ParameterId，并增加 ID 版本的 Set/Get API，字符串版本只是查询后转发。

状态和 Clip 也应在 Controller 绑定到 AnimationComponent 时解析和验证。第一版 State 可以继续保存 clipName；若之后发现每次进入 State 都在 `m_clips` 中查找，可以在 AnimationComponent 内维护已解析的 StateId 到 Clip 指针表，而不要把特定 Model 的 Clip 指针写回共享 Controller。

#### 手工 Play/CrossFade 与 Controller 的关系

Controller 模式和手工 Clip 控制不能默默同时工作，否则手工 `Play()` 后下一帧状态机仍认为自己位于旧 State，并可能立刻切回。第一版推荐采用互斥规则：

- 未设置 Controller 时，外部可直接使用 `Play()` 和 `CrossFade()`；
- 设置 Controller 后，游戏逻辑只修改参数或调用按状态名切换的调试接口；
- 若仍允许直接 `Play()`，它必须显式停用/清除当前 Controller 运行状态，或提供名为 `PlayClipOverride()` 的明显旁路 API；
- Controller 内部调用私有的底层播放/淡入函数，不能通过会停用 Controller 的公共 `Play()` 再绕一圈。

这种限制能避免“Clip 正在播放但 currentState 指向另一个 State”的双重真相。

#### Idle/Walk 最小示例

```cpp
auto controller = std::make_shared<AnimatorController>();
const auto speed = controller->AddFloat("speed", 0.0f);

const auto idle = controller->AddState({
    "Idle", "Idle", 1.0f, true
});
const auto walk = controller->AddState({
    "Walk", "Walking", 1.0f, true
});
controller->SetDefaultState(idle);

controller->AddTransition({
    idle,
    walk,
    0.2f,
    false,
    0.0f,
    0.0f,
    {{speed, AnimatorConditionOp::Greater, 0.1f}}
});

controller->AddTransition({
    walk,
    idle,
    0.2f,
    false,
    0.0f,
    0.0f,
    {{speed, AnimatorConditionOp::LessEqual, 0.1f}}
});

animation->SetController(controller);
```

角色控制器每帧只提交事实：

```cpp
animation->SetFloat("speed", glm::length(horizontalVelocity));
```

角色控制器不再直接判断 Idle/Walking 后调用 CrossFade。状态切换规则因此从 `ThirdPerspectiveController` 中移到共享 AnimatorController 配置中。

#### Validate 应检查的内容

Controller 自身至少检查：

- defaultState 有效；
- State 名称非空且唯一；
- Parameter 名称非空且唯一；
- State speed 是有限数值；
- Transition 的 source/destination StateId 有效；
- blendDuration 非负且为有限数值；
- normalized time 参数为有限数值并在允许范围内；
- Condition 的 ParameterId 有效；
- Condition 操作和参数类型匹配；
- threshold variant 类型正确；
- 无 Condition 且无 Exit Time 的立即 Transition 给出警告；
- 同一源 State 下完全相同的 Transition 给出警告。

绑定到 AnimationComponent 时额外检查：

- 每个 clipName 已注册；
- Clip duration 和轨道数据有效；
- Controller 默认状态可以实际进入。

第一版不需要检测所有可能的状态环，因为循环状态机本身合法；重点防止一帧内连续执行多条 Transition。每帧最多采用一条、过渡期间暂不重新决策即可避免无限循环。

#### 推荐编码顺序

1. 修正当前 `AnimationState` 草稿，改名为 `AnimationStateDefinition`，把 looping 改为 bool，并移到 AnimatorController 相关头文件。
2. 实现 ParameterDefinition、Condition 和 TransitionDefinition，以及 Controller 的 Add/Find/Get/Validate。
3. 为 AnimationComponent 增加 Controller、参数值数组、currentState 和 destinationState。
4. 实现 `SetController()` 的原子化绑定与默认状态进入。
5. 实现无 Exit Time 的 Bool/Float 条件切换，复用当前 CrossFade。
6. 增加 Int、Trigger，并确保只在成功采用 Transition 后消费 Trigger。
7. 增加 normalized Exit Time 和 destination start offset。
8. 用两个使用同一 Controller 的模型实例测试参数、状态和播放时间互不影响。
9. 状态机稳定后再开始第三阶段 1D Blend Tree。

建议准备以下测试场景：Idle 与 Walk 双向切换、过渡期间参数反复跨阈值、非循环 Attack 自动返回 Idle、相同 Trigger 连续触发、暂停时状态机冻结、两个实例设置不同 speed，以及目标 Clip 缺失时不破坏当前播放。

### 第三阶段：Blend Tree

Blend Tree 解决的是“一个状态内部如何根据连续参数组合多个 Motion”，状态机解决的是“离散状态之间何时切换”，两者不要合并成一个概念。例如 Locomotion 状态内部可用 `speed` 在 Idle/Walk/Run 间连续混合，而 Locomotion 到 Attack 由状态机 Transition 决定。

本阶段只实现 **1D Blend Tree**。目标是让一个 State 不再只能引用单个 Clip，而是引用一个 Motion；Motion 可以是单 Clip，也可以是由一个 float 参数驱动的 1D Tree。1D Tree 可以继续引用其他 1D Tree；暂不实现 2D Tree、动画层、事件混合和 Root Motion。

#### 概念和所有权

建议把只读定义和每个角色的运行时状态严格分开：

```text
AnimatorController（共享、只读定义）
  ├─ ParameterDefinition[]
  ├─ MotionDefinition[]
  │    ├─ ClipMotionDefinition
  │    └─ BlendTree1DDefinition
  ├─ AnimationStateDefinition[]   -- State 引用 MotionId
  └─ TransitionDefinition[]

AnimationComponent（每个实例的可变状态）
  ├─ ParameterValue[]
  ├─ current/destination StateId
  ├─ MotionPlayback               -- 当前 Motion 的相位和播放状态
  ├─ AnimationBlendTransition     -- 捕获的源 Pose + 目标 MotionPlayback
  └─ 最终 AnimationPose / SkeletonPose
```

`AnimatorController` 持有 Motion 定义，因为它和 State、Transition 一样是可共享的配置。`AnimationComponent` 只保存 Motion 播放进度和参数值。不要把 `time`、权重缓存或采样后的 Pose 放进 Controller，否则共享同一 Controller 的角色会互相影响。

#### 建议增加和修改的文件

第一版建议只新增一个头文件，不急着为求值器引入新类：

```text
engine/src/Renderer/AnimatorMotion.h                 新增：Motion 定义和 MotionId
engine/src/Renderer/AnimatorController.h/.cpp       修改：保存、创建、查询和验证 Motion
engine/src/Renderer/Animation.h                     修改：加入通用 MotionPlayback
engine/src/Scene/Components/AnimationComponent.*    修改：推进 Motion 时间并采样 1D Tree
docs/006-animation-architecture.md                  更新：本设计与实现结论
```

`AnimatorMotion.cpp` 只有在定义本身需要非平凡函数时再增加。第一版的二分查找、权重计算和 Pose 采样可以继续作为 `AnimationComponent` 的私有函数；当前只有一个消费者，立刻增加 `BlendTreeEvaluator` 会增加接口和所有权复杂度。如果以后节点类型增多、需要离线预览或单元测试独立求值，再抽出无状态的 `AnimationMotionEvaluator`。

#### Motion 定义

推荐使用稳定 ID 和 `std::variant`，而不是虚基类与大量堆分配：

```cpp
using AnimatorMotionId = uint32_t;
inline constexpr AnimatorMotionId InvalidAnimatorMotionId =
    std::numeric_limits<AnimatorMotionId>::max();

struct ClipMotionDefinition {
    std::string clipName;
};

struct BlendTree1DChild {
    float threshold = 0.0f;
    AnimatorMotionId motionId = InvalidAnimatorMotionId;
};

struct BlendTree1DDefinition {
    AnimatorParameterId parameterId = InvalidAnimatorId;
    std::vector<BlendTree1DChild> children;
};

using AnimatorMotionData = std::variant<
    ClipMotionDefinition,
    BlendTree1DDefinition>;

struct AnimatorMotionDefinition {
    std::string name;
    AnimatorMotionData data;
};
```

`AnimatorParameterId` 当前定义在 `AnimatorController.h`，而 Motion 又需要它。为避免循环包含，可以把 `AnimatorParameterId`、`AnimatorMotionId`、`InvalidAnimatorId` 和参数枚举移动到一个更基础的 `AnimatorTypes.h`；或者第一版把上述 Motion 类型直接放在 `AnimatorController.h`。不要让 `AnimatorMotion.h` 反向包含完整的 `AnimatorController.h`。

1D Tree 的 child 可以是 `ClipMotionDefinition` 或另一个 `BlendTree1DDefinition`。Controller 将 Motion 作为有向图验证：child MotionId 必须有效，并使用 DFS 的 visiting/visited 状态拒绝直接或间接成环。当前公开 Add API 只能引用已经创建的 Motion，因此通常自然形成 DAG；环检测仍作为加载器或未来编辑接口修改定义时的最终安全边界。

`AnimationStateDefinition` 改为引用 Motion：

```cpp
struct AnimationStateDefinition {
    std::string name;
    AnimatorMotionId motionId = InvalidAnimatorMotionId;
    float speed = 1.0f;
    bool looping = true;
};
```

这会替代当前 `clipName`。不要同时长期保留 `clipName` 和 `motionId` 两个真相来源。为方便现有代码迁移，Controller 可以提供便利接口：

```cpp
AnimatorMotionId AddClipMotion(std::string name, std::string clipName);
AnimatorMotionId AddBlendTree1D(std::string name,
    AnimatorParameterId parameter,
    std::vector<BlendTree1DChild> children);
AnimatorStateId AddState(AnimationStateDefinition state);

const AnimatorMotionDefinition* GetMotion(AnimatorMotionId id) const;
AnimatorMotionId FindMotion(std::string_view name) const;
```

也可以临时提供 `AddClipState(stateName, clipName, ...)`，让它内部先创建 ClipMotion 再创建 State，但它只能是便利函数，不应形成第二套存储。

#### 1D 权重算法

Tree 创建或验证完成后，children 必须按 threshold 严格递增。可以在 `AddBlendTree1D()` 中排序后保存，并在发现重复 threshold 时拒绝；这样运行时无需处理零分母。

对于输入参数 `x`：

1. 没有 child：定义非法，Controller 验证失败；
2. 只有一个 child：权重恒为 1；
3. `x <= first.threshold`：只采样第一个 child；
4. `x >= last.threshold`：只采样最后一个 child；
5. 否则用 `std::lower_bound` 找到第一个 `threshold >= x` 的右 child，左 child 是它的前一个；
6. 计算 `t = (x - left.threshold) / (right.threshold - left.threshold)`；
7. 只采样左右两个 child，并执行 `BlendPose(leftPose, rightPose, t)`。

边界恰好落在某个 threshold 时，应只采样该 child，避免无意义地采样两次。输入、threshold 和计算出的权重都必须是有限数；权重最终 clamp 到 `[0, 1]`。不要每帧复制整个 children 数组，也不要为了第一版计算所有 child 的权重。

例如：

```text
Idle threshold = 0.0
Walk threshold = 2.0
Run  threshold = 6.0

speed = 1.0  -> Idle 0.5 + Walk 0.5
speed = 4.0  -> Walk 0.5 + Run  0.5
speed = 8.0  -> Run  1.0
```

混合必须复用现有 LocalPose 路径：平移和缩放 `mix`，旋转使用归一化后的 `slerp`。不能混合最终 `skinMatrices`。一个 Tree 每帧最多产生两个完整 LocalPose，之后只对混合结果执行一次全局节点传播和 palette 生成。

#### 统一 normalized-time 时钟

同一 Locomotion Tree 中的 Idle、Walk、Run 通常要保持脚步相位一致。不能为三个 child 各维护一套独立、自由推进的秒数时间，否则参数变化时可能从左脚支撑突然跳到右脚支撑。

因此 State/Motion 运行时应持有一个统一的 normalized phase，而不是把单个 `AnimationPlayback::clip` 当成唯一时间源：

```cpp
struct MotionPlayback {
    AnimatorMotionId motionId = InvalidAnimatorMotionId;
    float normalizedTime = 0.0f; // looping 时保存 [0, 1)，非循环时 [0, 1]
    float speed = 1.0f;
    bool looping = true;
    bool playing = false;
};
```

采样 Clip child 时再映射：

```text
clipTime = normalizedTime * clip.duration
```

推进相位需要当前 Motion 的“有效时长”。单 Clip 的有效时长就是 clip duration；1D Tree 使用当前左右 child 时长按相同权重线性插值：

```text
effectiveDuration = lerp(left.duration, right.duration, weight)
normalizedDelta   = deltaTime * stateSpeed / effectiveDuration
```

这样 `speed == 1` 时，一个混合周期大致保持为当前动作组合的自然周期，参数变化时 normalized phase 连续。`effectiveDuration <= 0`、非有限数或 child clip 无效时必须停止/拒绝求值，不能除零。

这是第一版清晰且可预测的策略，但它不能自动保证不同素材的脚步接触点完全一致。资产应尽量让周期动作从相同相位开始；以后可以增加同步标记或基于步态事件的 phase matching。

第一版建议规定 BlendTree1D 只用于循环动作，并要求它的所有 child clip 可循环。攻击、死亡等一次性动画仍使用 ClipMotion。若允许非循环 Tree，还要定义不同 duration child 到达结尾后的保持、事件和退出语义，本阶段没有必要承担这些复杂度。

#### 对现有 CrossFade 和状态机的影响

目前 `AnimationBlendTransition` 的 destination 是 `AnimationPlayback`，只能指向一个 Clip。应改为 `MotionPlayback`：

```cpp
struct AnimationBlendTransition {
    AnimationPose sourcePose;
    MotionPlayback destination;
    float elapsed = 0.0f;
    float duration = 0.2f;
    bool active = false;
};
```

进入 State 时：

1. 从 State 取得 `motionId`；
2. 创建目标 `MotionPlayback`，写入 State 的 speed、looping 和 Transition 的 `destinationStartNormalized`；
3. CrossFade 开始时仍捕获“当前最终输出 Pose”为 `sourcePose`；
4. 过渡期间，每帧用**当前参数值**求值目标 Motion，因此玩家在淡入 Locomotion 时改变 speed，目标 Pose 和有效时长可以立即响应；
5. 将目标 Motion Pose 与捕获的 sourcePose 按过渡进度混合；
6. 完成后把 destination 提升为当前 MotionPlayback。

状态机的 Exit Time 使用 MotionPlayback 的 normalized time，不再读取某个具体 child clip 的秒数。这样 Locomotion 的退出条件不会因为当前混合到 Walk 或 Run 而改变定义。现有“循环时只判断 `normalizedTime >= exitTime`”仍可能漏掉跨帧和跨循环，应继续按前文建议记录 previous normalized time/循环计数；这与 Blend Tree 无关，但在改造统一时钟时适合一并修正。

手动 `Play()` / `CrossFade()` 仍可保留，它们内部创建临时 ClipMotion 播放请求或走专用的 Clip 入口，并继续与 Controller 模式互斥。不要让手动播放偷偷改写共享 Controller 的 Motion 表。

#### AnimationComponent 内部接口建议

第一版无需增加公开接口，主要替换私有求值函数：

```cpp
struct MotionSample {
    AnimationPose pose;
    float effectiveDuration = 0.0f;
};

bool ResolveBlendTree1D(const BlendTree1DDefinition& tree,
    float parameterValue,
    /* out: left/right motion and weight */) const;

bool SampleMotion(AnimatorMotionId motionId,
    float normalizedTime,
    MotionSample& output) const;

bool AdvanceMotion(MotionPlayback& playback, float deltaTime);
```

`SampleMotion()` 只负责生成完整 LocalPose 和有效时长，不生成 skinning palette，也不修改 GameObject。每帧的推荐顺序为：

```text
读取本实例参数
  -> 解析当前/目标 Motion 的两个 active child
  -> 根据有效时长推进统一 normalized time
  -> 采样 child clip 为 LocalPose
  -> Tree 内部混合
  -> 若处于 State Transition，再与捕获的 sourcePose 混合
  -> 写一次节点 binding
  -> 构建一次 skinning palette
```

为了让“推进时间”和“采样 Pose”使用完全一致的 child 与权重，可以让 Resolve 结果成为一次 `OnUpdate()` 内的局部值，或由 `EvaluateMotion()` 同时返回有效时长和 Pose。不要把它缓存成跨帧共享状态；参数每帧都可能变化。

#### Controller 验证规则

在现有 `Validate()` 基础上至少增加：

- Motion 名称非空且唯一，MotionId 有效；
- ClipMotion 的 clipName 非空；绑定到 `AnimationComponent` 时还要确认 clip 已注册；
- BlendTree1D 的 parameterId 有效且类型必须为 Float；
- Tree 至少有一个 child；
- 所有 threshold 有限且严格递增，不允许重复；
- child MotionId 有效，允许指向 ClipMotion 或另一个 1D Tree，但整个 Motion 图不能成环；
- State 的 motionId 有效；
- 第一版 BlendTree State 必须 looping，绑定时所有 child clip duration 必须为有限正数；
- 同一个 Controller 中名称重复的 State、Parameter 和 Motion 都应拒绝，而不是静默返回已有对象。

结构验证放在 `AnimatorController::Validate()`；需要访问实际 clip 注册表的验证放在 `AnimationComponent::SetController()`。`SetController()` 仍应保持原子性：先验证全部 Motion/Clip，再提交 Controller 和运行时状态，失败时保留原播放状态。

#### 最小使用示例

```cpp
auto controller = std::make_shared<AnimatorController>();
const auto speed = controller->AddFloat("speed", 0.0f);

const auto idle = controller->AddClipMotion("IdleMotion", "Idle");
const auto walk = controller->AddClipMotion("WalkMotion", "Walking");
const auto run  = controller->AddClipMotion("RunMotion", "Running");

const auto locomotionMotion = controller->AddBlendTree1D(
    "LocomotionMotion",
    speed,
    {
        {0.0f, idle},
        {2.0f, walk},
        {6.0f, run}
    });

const auto locomotion = controller->AddState({
    "Locomotion", locomotionMotion, 1.0f, true
});
controller->SetDefaultState(locomotion);

animation->SetController(controller);
animation->SetFloat("speed", glm::length(horizontalVelocity));
```

参数单位由游戏定义。上例 threshold 使用世界速度；引擎只把它当作 float，不应假设米/秒，也不应在 AnimationComponent 内读取角色速度。

#### 推荐实施顺序

1. 增加 MotionId、ClipMotionDefinition、BlendTree1DDefinition 和 Controller 的 Motion API/验证，不修改采样路径；先把现有每个 State 的 clipName 迁移为 ClipMotion。
2. 把 `AnimationPlayback` 的单 Clip 时间模型改为 `MotionPlayback` 的 normalized time，确认单 Clip State、CrossFade、speed、looping 和 Exit Time 行为不退化。
3. 实现 BlendTree1D 的排序/验证与左右 child 查找，并用纯数值测试覆盖阈值外、阈值上、区间内、单 child 和非法重复 threshold。
4. 复用现有 `SampleLocalPose()` 和 `BlendPose()` 实现两个 ClipMotion 的 Pose 求值，只生成一次 palette。
5. 将 Blend Tree 接入 State Transition 的目标 Motion，验证过渡中修改参数不会跳回源 Clip，也不会提前提交 destination State。
6. 用两个共享同一 Controller 的模型实例测试不同 speed 参数、normalized phase 和状态互不影响。
7. 最后处理 previous normalized time/循环计数，为准确 Exit Time、动画事件和未来 Root Motion 打基础。

建议准备以下场景：Idle/Walk/Run 连续加减速、输入恰好命中 threshold、瞬间从 0 跳到最大值、CrossFade 进入/离开 Locomotion、暂停后恢复、负 state speed（若继续支持倒放）、不同 duration 的 Walk/Run、缺失 child clip，以及两个实例共享 Controller。

完成这一阶段的判断标准不是“可以看到两个动画混在一起”，而是：State 统一引用 Motion；单 Clip 与 1D Tree 走同一 normalized-time 和 Pose 输出路径；Controller 完全只读可共享；每个 AnimationComponent 的参数、相位和过渡独立；每帧最终只写一次节点姿态并生成一次 palette。

2D Freeform/Directional Blend 的权重算法和边界情况明显更复杂，应等 1D、时间同步和事件语义稳定后再做。

同一 Blend Tree 中的周期动作通常应使用 normalized time 同步，否则 Walk 和 Run 的左右脚相位可能相反。各子 Motion 仍保留自己的 duration，采样时间由统一 normalized time 映射为 `normalizedTime * clip.duration`。

#### 当前已落地的 1D Blend Tree

当前实现已经完成上述第一阶段边界：Animator 的公共 ID、参数、State、Transition 和 Motion 定义集中在 `AnimatorTypes.h`；`AnimatorController` 保存 ClipMotion/BlendTree1D 的只读定义，并在添加及最终 `Validate()` 时检查 float 参数、有限且不重复的 threshold、有效 child Motion、State Motion，以及嵌套 Motion 图无环。

`AnimationComponent` 使用每实例 `MotionPlayback::normalizedTime`。ClipMotion 以 `normalizedTime * clip.duration` 采样；BlendTree1D 只查找参数两侧的两个 ClipMotion，混合完整 LocalPose。相位推进采用两侧 Clip duration 按同一权重插值得到的 effective duration，因此参数变化不会建立多套互相漂移的播放时钟。CrossFade 的 destination 也已改为 MotionPlayback，所以单 Clip 和 Blend Tree 都能作为状态及过渡目标。

手动 `Play/CrossFade` 仍按 clipName 查找对应 ClipMotion，但不再销毁提供 Motion 定义的 Controller；它只退出状态机模式。`SetController()` 在提交新 Controller 前完成结构、Motion 图及递归 clip/duration 验证，失败不会破坏当前播放状态。BlendTree1D 的 child 可以继续指向 BlendTree1D；Pose 和 effective duration 都使用相同 Motion 图递归求值。当前仍要求 Blend Tree State 循环播放；精确跨循环 Exit Time、同步标记、事件与 Root Motion 留待后续阶段。

### 第四阶段：动画层和骨骼 Mask

动画层用于同时表达全身移动、上半身攻击、面部或受击等相互叠加的动作。每层建议包含：

```cpp
enum class LayerBlendMode { Override, Additive };

struct AnimationLayerRuntime {
    // 每层可以拥有一个状态机运行实例。
    float weight = 1.0f;
    LayerBlendMode mode = LayerBlendMode::Override;
    // optional BoneMask
};
```

层的计算顺序固定为从底层到高层：

- Override：`result = Blend(result, layerPose, layerWeight * boneMaskWeight)`；
- Additive：将 layerPose 相对参考 Pose 的平移差、旋转差和缩放比叠加到 result；
- BoneMask：为每个 ModelNode/Bone 提供 `[0, 1]` 权重，例如上半身层只影响 spine 及其后代。

第一版动画层只实现 Override + BoneMask 即可。Additive 动画要求明确参考姿势，并正确处理四元数差值和缩放，因此适合后续实现。

### 事件、Root Motion 与过渡语义

混合加入后，事件不能简单地从“最终 Pose”推导，因为 Pose 不包含事件。每个 Playback 应独立推进时间并报告跨过的事件，然后由 Animator 决定是否派发：

- 第一版可以只派发目标状态事件；
- 更完整的实现可按状态权重阈值过滤源和目标事件；
- Trigger/攻击命中等关键逻辑事件不应因为低帧率或跨循环而漏发；
- 过渡被打断时，已经派发的事件不能重复发送。

Root Motion 也应在各输入 Clip 采样后先计算根节点增量，再使用与 Pose 相同的权重混合，最后交给角色控制器应用；应用后的根位移要从骨架 Pose 中移除，避免移动两次。基础 CrossFade 阶段可以暂不实现，但应继续记录每个 Playback 的 previousTime。

### 推荐实施顺序

1. 引入 `LocalTransform` 与完整 `AnimationPose`，让单 Clip 也经过“采样 Pose -> 生成 palette”的统一路径。
2. 实现可被打断的两输入 `CrossFade()`，先采用线性权重。
3. 把当前 Clip、time、speed、looping 收拢为 `AnimationPlayback`，补充 normalized time 工具。
4. 增加共享 `AnimatorController` 与每实例状态机，状态的 Motion 第一版只允许 Clip。
5. 实现 1D Blend Tree，并为周期动作加入 normalized-time 同步。
6. 增加 Override Animation Layer 和 BoneMask。
7. 最后实现 Additive Layer、复杂 2D Blend Tree、事件混合策略、Root Motion 和动画重定向。

不要一开始同时实现状态机、Blend Tree、Layer 和事件系统。最关键的稳定边界是：所有输入最终都生成完整 LocalPose，所有混合都发生在 LocalPose 上，而全局骨骼矩阵和 skinning palette 每帧只生成一次。
