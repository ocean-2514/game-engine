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

### 第三阶段：Blend Tree

Blend Tree 解决的是“一个状态内部如何根据连续参数组合多个 Motion”，状态机解决的是“离散状态之间何时切换”，两者不要合并成一个概念。例如 Locomotion 状态内部可用 `speed` 在 Idle/Walk/Run 间连续混合，而 Locomotion 到 Attack 由状态机 Transition 决定。

建议抽象一个只读 Motion 定义：

```text
Motion
  ├─ ClipMotion
  └─ BlendTree
       ├─ 1D Blend：一个 float 参数，如 speed
       ├─ 2D Blend：两个 float 参数，如 velocityX/velocityY
       └─ Direct Blend：外部直接给每个子 Motion 权重（以后实现）
```

先实现 1D Blend Tree：按 threshold 排序，找到参数两侧的两个子 Motion，只采样并混合这两个输入。2D Freeform/Directional Blend 的权重算法和边界情况明显更复杂，应等 1D、时间同步和事件语义稳定后再做。

同一 Blend Tree 中的周期动作通常应使用 normalized time 同步，否则 Walk 和 Run 的左右脚相位可能相反。各子 Motion 仍保留自己的 duration，采样时间由统一 normalized time 映射为 `normalizedTime * clip.duration`。

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
