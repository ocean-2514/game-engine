# Component 架构

- 状态：当前实现约定
- 最后更新：2026-08-13

## 所有权与创建

`GameObject` 通过 `unique_ptr<Component>` 独占组件。公开接口返回的 `Component*` 是非拥有观察指针；组件被实际释放后该指针失效。

组件通过 `GameObject::AddComponent<T>()` 创建和挂接。`Component` 的基类构造函数为 protected，生命周期回调也由 `GameObject` 统一调用，不作为普通外部接口使用。

## Update 与 Render

组件分别覆盖 `OnUpdate(float)` 和 `OnRender(RenderQueue&)`。`MeshComponent` 只向传入的队列提交渲染命令，不依赖 `Engine` 单例。

Scene 的 Update 与 Render 已经拆分。`OnUpdate` 仅由 `Scene::Update()` 驱动；`OnRender` 在 `Scene::Render()` 建立 RenderView 后驱动。MeshComponent 只提交 Mesh、Material 和 model matrix，不接收或保存具体 Camera；相机归属由当前 RenderView 批次表达。

CameraComponent 负责根据 owner 的世界变换计算 view matrix，并根据投影参数与调用方提供的 aspect 计算 projection matrix。它不直接访问 Shader 或 RenderQueue。

## 组件类型判断

组件使用进程内轻量 TypeId 做精确类型查询：

```cpp
class MeshComponent : public Component {
public:
    ENG_COMPONENT_TYPE(MeshComponent);
};
```

每种组件类型通过 `Component::GetStaticTypeId<T>()` 延迟取得唯一 ID，实例通过虚函数 `GetTypeId()` 返回相同 ID。`GameObject::GetComponent<T>()`、`HasComponent<T>()` 和 `RemoveComponent<T>()` 先比较 ID，匹配后使用 static_cast，不依赖 RTTI dynamic_cast。

该语义是具体类型精确匹配：查询基类不会返回它的派生组件。TypeId 只保证当前进程内唯一，不保证不同运行的数值稳定，不得写入存档、资源文件或网络协议。未来若使用动态 DLL/插件，还必须保证所有模块通过同一个引擎分配器取得 ID，或改用稳定名称/哈希/注册表。

## 延迟结构修改

组件遍历期间不能直接扩容 `m_components`，否则会使迭代器失效。`GameObject` 因此使用私有的强类型命令：

```text
GameObjectCommand = variant
└── AttachComponentCommand (持有 unique_ptr<Component>)
```

外部不接触命令类型，仍然只调用 `AddComponent<T>()`。非遍历期间立即挂接；组件回调期间新增的组件进入队列，在当前组件遍历完成后按提交顺序挂接。

删除不需要单独的命令类型。`RemoveComponent<T>()` 只调用 `MarkForDestroy()`：组件会立即进入 staged-dead 状态，`GetComponent`、`HasComponent` 和后续生命周期回调均忽略它；安全遍历点再实际释放。这样删除操作既不会使迭代器失效，也不会制造一份重复的延迟删除协议。

若组件回调将所属 `GameObject` 标记销毁，本对象剩余组件和子树的本阶段遍历立即停止。

## 当前边界

- 组件和对象的观察裸指针都不提供跨帧存活保证；需要长期保存时，调用方必须先依赖所有者生命周期并检查 `IsAlive()`。
- `AddComponent<T>()` 返回非空表示组件已经立即挂接，或其延迟挂接请求已被接受。遍历期间新组件不会参与当前这一次组件遍历。
- 当前命令队列是单线程设计，不提供并发提交保证。
- 暂不加入组件依赖声明、事件总线、序列化或系统式 ECS；等现有需求明确后再扩展。
- `Component` 基类构造为 `protected` 并不严格禁止外部直接实例化具有 `public` 构造函数的派生组件；它只保证外部无法把组件挂进 `GameObject`。若将来要在语法层面完全限制创建，需要让每个派生组件授权 `GameObject` 创建，同时调整模板构造方式，目前没必要增加这层复杂度。

## 未来可能设计（未实现）

- `ComponentHandle`：使用 index + generation 代替需要跨帧保存的裸观察指针，并解决主相机引用失效问题；
- 多组件查询：当前 `GetComponent<T>()` 只返回第一个精确类型，未来可增加 `GetComponents<T>()`；
- 多态类别查询：若真实需求要求按组件基类/接口查询，可单独建立类别位标志或类型继承注册关系，不改变当前精确 TypeId 语义；
- 启用状态：区分 alive（生命周期）与 enabled（是否参与 Update/Render），并决定对象禁用是否向子树传播；
- 组件依赖：例如 Renderer 需要 Transform/资源，采用运行时校验还是声明式 RequireComponent，待实际冲突出现后决定；
- 生命周期钩子：OnAttach、OnDetach、OnEnable、OnDisable、OnDestroy；只有在资源注册或系统订阅需要对称清理时再加入；
- Scene/系统注册：将可渲染、相机、物理等组件登记到紧凑列表，避免每个系统反复遍历整棵对象树；
- 序列化与反射：TypeId 不能充当稳定序列化 ID，需要独立的稳定类型名称、版本和字段描述；
- 并行更新：当前 GameObjectCommand 与组件容器均为单线程语义，未来需用阶段屏障、线程本地命令缓冲或场景快照重新设计。
