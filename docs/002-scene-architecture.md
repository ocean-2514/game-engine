# Scene 与 GameObject 架构

- 状态：已采纳
- 最后更新：2026-08-13

## 所有权模型

Scene 使用一棵由 `unique_ptr` 组成的拥有树：

```text
Scene
└─ root GameObject (unique_ptr)
   └─ child GameObject (unique_ptr)
      └─ child GameObject (unique_ptr)
```

`GameObject::m_parent` 是非拥有观察指针。Scene 和父节点是唯一所有者；公开接口返回的 `GameObject*` 也是观察指针，节点销毁后立即失效。

重新设置父节点时，Scene 从原容器提取已有 `unique_ptr`，再移动到目标父节点或根容器。禁止从观察裸指针重新构造 `unique_ptr`。

## 更新与销毁

树遍历由不可覆盖的 `GameObject::UpdateTree()` 控制，派生类只覆盖 `OnUpdate(float)`，避免派生类忘记调用基类 Update 而中断子树更新。

`MarkForDestroy()` 只修改存活标志，可以在 `OnUpdate()` 中安全调用。Scene 或父节点在遍历的安全位置删除失效节点，其整个子树随所有权递归销毁。如果对象在自身 OnUpdate 中标记销毁，本帧不会继续更新它的子节点。

## 延迟 Scene 操作队列

更新遍历期间的结构修改使用 Scene 私有的强类型命令队列：

```text
SceneCommand = variant
├─ AttachObjectCommand（持有新对象 unique_ptr）
├─ ReparentCommand（持有对象观察指针）
└─ ClearCommand
```

命令类型不向外暴露。外部仍然调用 `CreateObject`、`SetParent` 和 `Clear`；Scene 根据 `m_isUpdating` 决定立即应用还是 staged。

本轮遍历结束后，`m_isUpdating` 先恢复为 false，然后按提交顺序执行 `FlushPendingCommands()`。在此之前，遍历已经移除所有被标记销毁的对象。Immediate 方法必须始终先用 `Contains()` 验证观察指针，确认仍由 Scene 拥有后才能解引用。

## Staged 返回值语义

更新期间调用 `CreateObject`：

- 非空返回值表示新对象已由 `AttachObjectCommand` 持有；
- 对象地址在 `unique_ptr` 移动时保持稳定；
- Flush 前 `Contains(object)` 返回 false；
- Flush 前对象尚未接入父子树，名称和 parent 等附着状态也尚未最终应用；
- 如果前序命令使 parent 失效，Attach 在 Flush 时失败，对象随命令销毁，返回的观察指针立即失效。

更新期间调用 `SetParent`：

- `true` 表示请求通过当前基础校验并进入 staged 队列；
- 它不保证 Flush 时最终成功；
- 前序 Clear、对象销毁或其他层级操作可能让命令在执行时失效；
- Immediate 阶段会重新检查 Scene 归属、存活状态和循环父子关系。

Scene 将已挂接对象和待 Attach 对象统称为 known object。这样本轮刚创建的 staged parent 可以继续用于创建 staged child 或提交 Reparent。`Contains()` 仍只表达“已经接入场景树”，不包含 staged object。

## Clear 的有序语义

`Clear()` 明确定义为普通有序命令，不是本帧终止操作。例如：

```text
Attach A
Clear
Attach B（parent = null）
```

Flush 后 Scene 中保留 B。

如果 Clear 后的命令引用了被 Clear 删除的对象，它会在 Immediate 校验时失败。调用方必须把 Clear 之后持有的旧 `GameObject*` 视为可能失效。

更新阶段之外调用 Clear 会立即清空场景树。

## SetParent 约束

Immediate `SetParent(object, parent)` 仅在以下条件满足时成功：

- object 属于当前 Scene 且仍存活；
- parent 为空，或属于同一个 Scene 且仍存活；
- parent 不是 object 自身或其后代。

将 parent 设为空表示把节点提升为 Scene 根节点。更新期间的 staged SetParent 先验证 known object 和明显无效的自身父子关系，完整循环检查在按序 Flush 时进行。

## 当前限制与后续演进

命令只保留到本帧 Update 结束，当前可以接受使用观察裸指针。若未来命令需要跨帧、并行生成或序列化，应改用带 generation 的 `GameObjectHandle`。

staged 命令最终失败目前没有异步结果回传；公开返回值只表达“立即应用成功”或“staged 请求已接受”。若调用方需要获知最终结果，可以以后增加命令完成事件或操作票据。

Scene 结构修改仍限定在单线程。Render 和其他系统若要跨线程读取 Scene，需要另行设计快照或同步边界。

## 2026-08-13：Update、Render 与主相机

Scene 的更新与渲染提交已经拆分：

- `Scene::Update(deltaTime)` 只更新对象和组件、清理 staged-dead 对象并 flush SceneCommand；
- `Scene::Render(queue, aspect)` 使用 Scene 的主相机建立视图并遍历渲染树；
- `Scene::Render(queue, camera, aspect)` 显式指定相机，用于编辑器视角、小地图、反射或离屏视图；
- 真正的 GPU 执行仍由 Engine 在 Application::Render 结束后统一调用 `RenderQueue::Execute()`。

Scene 保存的 `m_mainCamera` 是默认选择，不表示 Scene 只能有一个 CameraComponent。CameraComponent 仍由 GameObject 独占；Scene 只保存非拥有观察指针。`SetMainCamera()` 验证组件存活、owner 存活且属于本 Scene，也允许传入 nullptr 清除默认相机。

当前裸指针方案存在明确限制：如果主 CameraComponent 被单独删除而 Scene 未先清除引用，`m_mainCamera` 会悬空，之后不能再通过 `IsAlive()` 安全验证。短期调用约定是先 `SetMainCamera(nullptr)` 或切换相机，再删除原相机组件/对象。

未来可以选择以下一种生命周期方案：

- 使用带 index + generation 的 `ComponentHandle`，查询时由 Scene/Registry 验证；
- 给 GameObject 增加所属 Scene 的非拥有引用，在组件销毁时通知 Scene；
- Scene 保存主相机 GameObject 的句柄，每次渲染重新查询其 CameraComponent；
- 建立组件 Registry，并让 Scene 保存稳定实体/组件句柄。

在句柄需求真正覆盖 GameObject、Component 和延迟命令以前，不单独为相机引入半套句柄系统。

Scene 当前掌管的是“由场景生成一个 RenderView 命令批次”的流程，不掌管 RenderQueue 的全局生命周期。Application 可以让多个 Scene 或同一 Scene 的多个 Camera 依次生成视图，Engine 最后统一执行队列。
