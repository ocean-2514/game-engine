# 架构导览

本文面向第一次阅读仓库的开发者，介绍当前代码的数据流与模块边界，不是对未来后端或尚未落地设计的承诺。

## 程序生命周期

```text
main
 └─ Engine::Init(Application*)
     ├─ 创建 GLFW Window / OpenGL RenderDevice
     ├─ 建立 AssetManager 与 AudioManager
     ├─ 初始化 RenderQueue
     └─ Application::Init()

Engine::Run 每帧
 ├─ Window::PollEvents → InputManager 本帧事件
 ├─ Application::Update(frameDelta)
 ├─ AudioManager::Update()
 ├─ Application::Render(RenderQueue)
 ├─ RenderQueue::Execute(RenderDevice, framebufferViewport)
 └─ SwapBuffers / 清理瞬态输入
```

`Engine` 是应用级服务的拥有者；`Application` 是用户游戏逻辑的扩展点。`Engine::Init` 接收并接管 `Application*` 的所有权，应传入动态分配且之后不再自行释放的对象。示例当前使用 Engine 单例访问窗口、输入、渲染设备和资产服务；这方便原型开发，但会增加测试替身和多 Engine 实例的难度。

初始化顺序具有实际生命周期含义：先创建窗口和 OpenGL context，再创建 RenderDevice/AssetManager/RenderQueue 与 AudioManager，最后调用应用的 `Init()`。销毁时，游戏对象和共享 GPU 资源需要在 OpenGL context 仍然有效时释放。Engine 还把输入 callback 写入 InputManager：`PollEvents()` 后 `BeginFrame()` 冻结本帧有序事件，帧尾 `ClearState()` 清掉边沿状态和事件列表。

`Application` 只规定 `Init / Update / Render / Destroy`；它不拥有 Engine 服务。Game 目前显式持有 Scene 与 UIContext，因此示例也展示了应用层如何编排“输入路由 → Scene 更新 → Scene/UI 提交”。

### Window 与 Input

Window 抽象只暴露窗口生命周期、尺寸、cursor mode、回调和图形过程地址；GLFW 的事件 callback 不直接驱动 GameObject，而是向 InputManager 写状态和 InputEvent 值。InputManager 同时保留“当前是否按住”状态以及按下/释放边沿状态，以便既支持传统每帧轮询控制器，也支持 UI 消费鼠标事件序列。

窗口尺寸以逻辑坐标表示，framebuffer 尺寸以像素表示。鼠标事件记录逻辑窗口位置；viewport/GL 绘制使用 framebuffer 像素。DPI 比例与 viewport 原点由 ScreenView/UI 映射处理。输入队列是一个按帧快照，不是跨系统订阅式事件总线；本帧事件在 `PollEvents → BeginFrame` 后对应用可见，帧尾清除瞬态事件但保留按键持续状态。

## 场景与组件

Scene 拥有 GameObject 根树以及自己的 PhysicsWorld。GameObject 保存本地 Transform、父子层级和 Components。组件在 `OnAttach` / `OnDetach` 等生命周期中向 Scene 注册服务，而不是自己查找 Engine 单例。Scene 更新分为普通帧更新与固定物理更新；动态刚体结果同步回 Transform，渲染时可暂时应用物理插值。

组件通常在 `OnUpdate`、`OnFixedUpdate`、`OnRender` 等入口实现自身逻辑。Scene 在对象/组件遍历期间延迟执行结构变更，减少回调中销毁对象造成的遍历失效。开始修改生命周期代码时，应一并检查失败 Attach、Detach 和 Scene Clear 路径。

Scene 的对象容器由 `unique_ptr` 形成所有权树，Reparent 会从原容器提取再插入新父节点，避免复制对象。组件类型通过 `Component::GetStaticTypeId<T>()` 获取进程内类型 ID；派生组件以 `ENG_COMPONENT_TYPE` 提供查询入口。它是运行时查询机制，不是稳定的跨进程/资产类型 ID；场景 JSON 使用独立的字符串名称注册 Codec。

主循环每帧的物理调度可概括为：

```text
accumulator += frameDelta (clamp 到 maxAccumulatedTime)
while accumulator >= fixedTimeStep 且未超过 maxSubStepsPerFrame:
    fixed update → 推送 Kinematic → Bullet step → 拉回 Dynamic → 采集碰撞事件
    accumulator -= fixedTimeStep
若仍有过量积压：丢弃整步积压，保留不足一步的 remainder
普通帧 update(frameDelta)
```

当前默认配置使用 `1/60 s` 固定步长、每帧最多 8 个子步、最多累计 `0.25 s`。达到上限时丢弃过量整步是防止 spiral-of-death 的简单策略，会损失模拟时间；它不是高精度回放或网络确定性方案。Static 不随物理更新，Kinematic 由 Transform 推入物理世界，Dynamic 由物理结果回写；不应让游戏脚本和物理后端同时争夺同一动态物体的位置。

## 资源流

```text
AssetManager
 ├─ TextureAssetLoader → FileSystem / ImageLoader → RenderDevice::CreateTexture
 ├─ MaterialAssetLoader → JSON 描述 + Shader/Texture → Material
 ├─ ModelAssetLoader → Assimp 导入 → Mesh/Model/Material
 └─ SceneAssetLoader → Parse → Resolve dependencies → Instantiate Scene
```

资源路径通常是相对于 `assets/` 的规范化路径。解析描述（Desc）、加载依赖和创建运行时对象分开处理：Scene 的 ComponentCodec 负责 schema，Resolver 检查依赖，Instantiator 创建对象/组件。扩展序列化类型时优先复用注册表和 Codec，不要让 Component 直接依赖 RapidJSON。

Model 的几何与资源可以共享，GameObject/组件保存实例状态。贴图和材质由 AssetManager 管理加载缓存；要创建运行时可独立修改的材质，应确认调用 `InstantiateMaterial` 而不是修改共享缓存实例。

各加载阶段的大致失败边界如下：

1. **FileSystem** 规范化相对资产路径并读取文本/字节；UI、Physics、Renderer 不应各自重新拼绝对路径。
2. **Parser / Codec** 检查 JSON 结构、字段类型和注册的组件名称，产出纯描述对象及带 `sourcePath + jsonPath` 的错误。
3. **Resolver** 加载 Material、Model 等外部依赖，并在创建场景前发现缺失资源。
4. **Instantiator** 创建未挂载对象树与组件，处理对象 ID、相机/监听器等引用，全部有效后再提交到 Scene。

Texture cache key 不只是文件名，还含 sRGB、垂直翻转、mipmap 和 sampler 设置，因此相同图像以不同加载参数请求时不会误共用 GPU 资源。材质 JSON 把 SurfaceMode 与 RenderState 随材质保存；Model 导入时则由 Assimp 数据转换成引擎的 Mesh/Material/Model，而不直接把 `aiMaterial` 作为运行时绘制接口。导入材质和项目自定义着色器仍需要显式映射策略，不能假设任意 FBX/OBJ 在所有材质效果上自动一致。

## 渲染路径

前端使用 RenderDevice、Mesh/Texture/ShaderProgram、Material、RenderCommand 和 RenderQueue 等引擎类型。当前 `RenderDevice::Create()` 选择 OpenGL 实现；GLuint、OpenGL 调用和 GL 状态转换集中在 `Renderer/OpenGL/`。

Scene 为相机创建 3D RenderView 并提交对象绘制；RenderQueue 在 Execute 阶段排序并执行命令。Scene 2D 和 UI 使用 `ScreenViewDesc` / `ScreenQuadCommand` 的屏幕空间路径，不依赖相机/灯光的 3D 命令。当前队列有确定的世界与屏幕 pass 顺序；需要 UI 置顶时，应用应在其他 screen-view 内容后提交 UI。

渲染工作拆为“提交”和“执行”两段：Scene/Component 生成带 mesh、material、model transform、phase/order 的值命令；一个 RenderView 持有 camera、lighting、可选 framebuffer viewport 和命令数组；UI/Sprite2D 使用独立屏幕视图与 quad 命令。组件不直接绑定 GPU 状态，这让一次执行可以集中排序、验证命令并切换状态。

- 3D 排序以 RenderPhase、renderOrder 为主；Transparent 阶段在同 order 内按相机空间深度远到近稳定排序。透明排序是基于对象原点的启发式，不等价于三角形级排序，也不能正确解决所有相交透明网格。
- Material 保存 Shader、参数、纹理、SurfaceMode 和 RenderState。参数修改会增加 revision；提交时记录 revision，若同一帧提交后又修改该 Material，对应命令会被跳过，以避免同一命令列表中出现含糊状态。
- 同一个 Shader 的材质和视图数据采用有限缓存：材质参数在当前 material/revision 有效时复用；相机与灯光按 Shader/RenderView 更新；RenderState 在设备状态相同时尽量不重复设置。缓存需在外部改变 GL 状态后失效。
- 当前灯光 uniform 上限为方向光 2、点光 16、聚光 8；骨骼矩阵上限为 150。超出 shader 固定数组能力的方案尚未迁移到 UBO/SSBO 或其他后端资源。
- RenderQueue 执行所有 3D RenderView 后再执行 screen views。screen quad 可选择覆盖 Shader，但必须符合当前 `uModel/uProjection/uColor/uLowerLeftUV/uUpperRightUV/uTexture` uniform 约定；这不是任意 Shader 自动反射绑定。

在学习和扩展时，建议把“数据相同就能复用的 GPU 资源”与“每实例/每 draw 的数据”分开：Mesh/Texture/共享 Material 可复用，model matrix、tint、UV、骨骼姿态属于命令或实例。这样也解释了为什么 UI 颜色不应临时写进共享 3D Material。

该抽象主要用于隔离代码职责，不是完整跨 API 渲染架构：尚无 Vulkan 后端、RenderGraph、资源屏障或可序列化 GPU Pipeline。若要增加后端，应先从一个能力最小的端到端功能验证抽象，而不要把 Vulkan API 原样复制进前端。

## 物理、动画与音频

- `PhysicsWorld` 是 Scene 级服务，当前通过 PImpl 边界封装 Bullet。公共 API 使用 PhysicsBodyHandle、描述和值类型；Bullet 类型留在后端。物理按固定步长模拟，事件由 Scene 更新期间累积并在之后轮询。
- `AnimationComponent` 对动画播放/状态机进行求值；Model 提供共享骨骼/动画资源，SkinnedMeshComponent 将姿态数据提交到渲染路径。AnimatorController 可定义参数、状态、Transition 与 1D Blend Tree。
- `AudioManager` 是 Engine 级服务，AudioComponent 表示播放实例，AudioListenerComponent 参与选择监听位置。miniaudio 实现不应泄漏到游戏层。

### Physics 细节

`RigidBodyDesc` 通过 ColliderDesc 列表表达一个刚体可由多个本地偏移形状构成的 compound collider。动态刚体不接受当前的静态三角网格形状；碰撞层/掩码在创建 Bullet body 时传入。Body Handle 带 index 和 generation，避免 Bullet slot 复用后旧组件句柄继续操作新刚体。TriangleMesh backing 数据与 shape 按 BodySlot 生命周期共同持有。

碰撞事件在每个 fixed step 的 Bullet 模拟后，从 manifold 中拷贝为引擎值类型；事件队列在 Scene 下一次 Update 起始处重置，当前帧所有固定子步事件都可在 Update 返回后轮询。事件没有把 Bullet manifold 指针暴露给游戏层，也尚未自动转换成每个 Component 的 `OnCollision...` 回调。一个画面帧可能经历零个、一个或多个物理步，因此帧事件与“最近物理接触状态”不是同一种数据。

渲染插值只在 Render 阶段临时覆盖 Dynamic GameObject 的 Transform，作用域结束时还原模拟状态。Teleport 会重设 previous/current transform，避免插值路径穿过旧位置。物理 shape cache、约束、CharacterController、debug draw 和确定性网络回放尚未实现。

### Animation 细节

Model 保存节点层级、Bone inverse bind matrix 和 AnimationClip；多个实例可共享只读模型数据，每个 AnimationComponent 拥有独立 playback、参数、当前 pose 与 `SkeletonPose`。Assimp 导入把节点轨道映射到 ModelNode index。每次求值按 clip 时间采样 position/rotation/scale keyframe，恢复未动画轨道的 bind transform；交叉淡化与 1D Blend Tree 在本地姿态层混合，再从根节点递归计算全局变换和 skinning palette。

AnimatorController 把 Motion、State、Transition 与 Float/Bool/Int/Trigger 参数分开描述。Transition 可有多条件、Exit Time、目标开始时间及 blend duration。1D Blend Tree 以阈值排序子 Motion，根据浮点参数找到相邻两个子项并在姿态之间插值；支持嵌套 Motion 的采样/有效时长。这一系统当前由 C++ 构造 Controller 为主，编辑器、控制器 JSON 序列化、动画事件和根运动还未接入。

当前默认 Shader 通过每顶点四个 bone index/weight 做线性混合蒙皮，palette 以 uniform 数组上传，最大 150 根骨骼。这个规模适合学习和小型资源，但现代生产渲染通常会考虑骨骼调色板缓冲、每网格 palette 映射及设备上限验证。

### Audio 细节

Engine 独占一个 AudioManager / miniaudio engine；Scene 将 AudioManager 注入 AudioComponent，组件在 attach 时创建 AudioVoiceHandle，在 detach 时销毁 voice。句柄包含槽位代数，API 只暴露播放/暂停/停止/音量/播放状态等引擎语义。`AudioSourceDesc` 包含路径、Decode/Stream 模式、loop、play-on-attach、音量/pitch 与空间衰减配置；声源和 listener Transform 从 Scene 推送给音频服务。

`ma_engine` 和 `ma_sound` 由 AudioManager 的实现层 RAII 持有，音频回调线程由 miniaudio 管理。游戏层不应直接访问 ma 结构。当前一个 Engine service 意味着多 Scene 共享设备；Scene 负责绑定其声源/活动 listener，但音频 bus、资源资产缓存、混响及混音器仍未实现。

三个系统的线程、时间和生命周期不同，当前没有强制的全局 EventBus。优先使用所有权清晰的组件接口和值事件队列；若加入回调，需定义安全派发点和对象失效策略。

## UI 边界

UIContext 独占 UIElement 树，与 Scene 的世界对象树分离。元素以带 generation 的本地句柄引用；遍历期结构变更延迟执行。UIContext 将 InputEvent 坐标映射到 ScreenView，执行命中、捕获和元素事件分发，再把屏幕四边形值命令交给 RenderQueue。UIImage 持有 Texture 共享引用；UIButton 复用图片呈现并在按下/释放规则满足后通知点击监听器。

当前 UI 仅是基础层和示例控件，不含自动布局、裁剪、文本/字体、键盘焦点、导航或复杂主题。应用应使用 UICapture 控制 UI 与 gameplay 是否同时消费指针输入，并在窗口失焦时取消捕获。

`UIRect` 是父节点坐标系中的逻辑像素矩形；布局当前是绝对位置与尺寸，不会根据父尺寸自动 stretch。屏幕鼠标逻辑坐标先按 logical window/framebuffer 比例换算，再扣除 viewport 偏移并转到 UI view；TopLeft/BottomLeft view 均有明确原点语义。RenderOrder 影响绘制顺序，命中测试逆序查找顶层交互元素。隐藏节点不绘制/命中，禁用节点仍绘制但不交互，删除/失焦时向捕获节点发 Cancel。

UIButton 的点击是同一个 capture 手势上 Down 后再 Up，释放仍在元素及 view 内才通知；仅有 Up 不会误触发。背景接口复制 UIImage 当下的 texture/UV/tint，而不保存另一个控件裸指针。点击 listener 修改列表采用当前通知快照。UIContext 返回 UICapture，但示例角色控制器仍直接读取全局 InputManager，当前 Game 还没有完整把 UI 捕获结果路由给 gameplay，这是可验证的集成缺口。

UIContext 需要显式在其他屏幕视图之后提交，才能覆盖其上；Engine/Application 没有内建的全局 UIContext 生命周期。当前按钮不含自动 Hover/Pressed/Disabled 主题、label 文本、键盘导航或无障碍语义。

## 扩展约定

1. 先明确资源/运行时对象的所有者，再定义 Handle 或 shared_ptr 的使用范围。
2. 公共头文件优先使用引擎值类型；第三方后端结构留在对应实现边界。
3. 遍历期间若允许销毁/重挂载，使用有序延迟命令，并定义返回句柄处于 staged 还是 live。
4. 资源加载分为解析、依赖解析和实例化；错误应带来源路径或字段路径。
5. 新特性同时补充无设备逻辑测试与必要的可视化/运行验收说明。
6. 讨论稿是设计背景，不自动等于最终契约；变更代码后要以实际实现更新使用文档。

## 示例运行时可以观察到什么

示例游戏当前用键盘 `W/A/S/D` 移动角色，`Space` 跳跃，鼠标移动控制第三人称视角，滚轮调整视野；`Q/E` 调整竖直移动，鼠标右键在角色移动时切换行走/奔跑状态。UI 示例按钮位于 screen view 左上角，点击后向控制台输出消息。键位来自 `src/Game/Scripts/ThirdPerspectiveController.cpp` 和 `CameraComponent`，属于游戏示例代码，不是 Engine 全局输入映射系统。

当前 UI 会产生 pointer capture 结果，但第三人称控制器直接读取全局 InputManager，因此 UI 与镜头输入可能仍同时响应。若用此示例展示 UI 输入仲裁，应先将 UICapture 传递到 gameplay 输入路径并验证行为。
