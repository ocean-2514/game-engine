# 场景资产与实例化架构

- 状态：设计建议，尚未实现
- 最后更新：2026-10-07

## 目标与边界

场景文件首先是一种资产描述，不应直接等同于运行时 `Scene` 的内存布局。建议把流程拆成：

```text
JSON 文件
  -> SceneAssetLoader：语法解析、字段校验
  -> SceneDesc：纯 CPU、无运行时对象的中间描述
  -> 依赖解析：Material / Model / Texture 等资产
  -> SceneInstantiator：创建 GameObject 和 Component
  -> 全部对象创建后解析跨对象引用
  -> 接入 Scene，触发 OnAttach
```

这一边界使 JSON 格式升级、错误诊断、编辑器、Prefab 和异步加载不必侵入 `GameObject`、`Component` 或 Bullet/OpenGL 后端。

第一版应把“场景模板加载”和“运行时存档”视为不同问题。场景资产描述初始布局和资产引用；存档还需要保存运行状态、动画时间、速度、脚本数据等，不应现在就让 scene JSON 承担两种语义。

## 对当前示例格式的主要问题

`assets/scenes/scene1.json` 可以表达方向，但建议先修正以下问题：

1. 标准 JSON 不允许 `//` 注释。不要依赖 RapidJSON 的注释扩展，否则其他工具、Schema 校验器和编辑器可能无法读取。说明文字放入文档；确实需要时可采用显式 `_comment` 字段，但不建议把它作为常规格式。
2. `type` 当前同时表示 GameObject 派生类型、模型文件格式和 Component 类型，语义冲突。特别是 `"type": "obj"` 把模型格式泄漏到了场景结构中；加载器应根据模型资产路径处理格式。
3. `Camera` 更适合表示为普通 GameObject 加 `CameraComponent`，而不是特殊对象类型。只有真正包含对象级行为的游戏类才需要 GameObject 工厂。
4. 缺少顶层 `version`，以后字段改名、旋转格式变化或组件升级时无法迁移。
5. 缺少稳定 `id`，无法可靠表达主相机、父子关系以及组件之间的对象引用。名字可重复，不应作为持久引用主键。
6. 没有明确 children、enabled、main camera、物理世界配置等场景级字段。
7. `RigidBodyComponent` 只有 mass 不足以创建有效刚体；至少需要 motion type 和 collider 描述。不能根据 `mass > 0` 隐式推断所有语义。
8. Mesh 的 primitive 描述、外部 Model 实例和普通组件混在同一层级，后续验证会变得含糊。

## 推荐的第一版 JSON

可以采用如下结构：

```json
{
  "version": 1,
  "name": "scene1",
  "settings": {
    "mainCamera": "main-camera",
    "physics": {
      "gravity": [0.0, -9.81, 0.0],
      "fixedTimeStep": 0.016666667
    }
  },
  "objects": [
    {
      "id": "floor",
      "name": "Floor",
      "transform": {
        "position": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0, 1.0],
        "scale": [10.0, 1.0, 10.0]
      },
      "components": [
        {
          "type": "MeshComponent",
          "mesh": {
            "kind": "primitive",
            "primitive": "cube",
            "size": [1.0, 1.0, 1.0]
          },
          "material": "material/container.json"
        },
        {
          "type": "RigidBodyComponent",
          "motionType": "static",
          "mass": 0.0,
          "colliders": [
            {
              "shape": "box",
              "halfExtents": [5.0, 0.5, 5.0],
              "isTrigger": false
            }
          ]
        }
      ]
    },
    {
      "id": "main-camera",
      "name": "MainCamera",
      "components": [
        {
          "type": "CameraComponent",
          "fov": 45.0,
          "near": 0.01,
          "far": 100.0
        }
      ]
    },
    {
      "id": "backpack",
      "name": "Backpack",
      "instance": {
        "kind": "model",
        "asset": "model/backpack/backpack.obj"
      }
    }
  ]
}
```

格式约定：

- 所有资产路径相对于 assets 根目录，并经过现有 FileSystem 规范化；
- transform 缺失字段使用单位默认值；
- rotation 明确为局部四元数 `[x, y, z, w]`，导入时验证有限且非零后归一化；
- 数组比 `{x,y,z}` 更紧凑，但两者只能选一种规范，不建议长期同时接受；
- `instance` 表示由外部资产展开对象层级，`class` 才表示 GameObject 派生类型，两者不要复用 `type`；
- 第一版可让 children 直接嵌套；若需要任意跨层级 reparent，再增加 `parent` ID。不要同时把嵌套和 parent ID 作为两套权威层级来源；
- 未知字段第一版建议报 warning，未知 `type`、错误字段类型、重复 ID、重复单例组件和非法资产路径必须报 error。

## 不建议给每个运行时类增加 ParseProperties

不建议在 `GameObject` 和 `Component` 基类中增加：

```cpp
virtual bool ParseProperties(const rapidjson::Value& value);
```

主要原因：

- 运行时 Scene 层会直接依赖 RapidJSON；
- 类同时承担行为、格式版本迁移、错误诊断和资产加载；
- `MeshComponent`、`RigidBodyComponent` 等没有统一的默认构造条件；
- 属性必须在 `OnAttach()` 前准备完整，否则 RigidBody 会用半成品 Desc 创建后端 Body；
- 文件格式一变化就需要修改运行时类，测试也必须构造 JSON DOM；
- 将来使用二进制场景、编辑器或网络复制时会重复一套解析逻辑。

推荐为每种可序列化类型提供资产层 codec。codec 负责把 JSON 转成构造描述，工厂负责用描述和加载上下文创建对象：

```text
RigidBody JSON
  -> RigidBodyComponentCodec::Parse()
  -> RigidBodyDesc
  -> make_unique<RigidBodyComponent>(desc)

Mesh JSON
  -> MeshComponentCodec::Parse()
  -> MeshComponentDesc（primitive/model/material 引用）
  -> 通过 AssetManager/RenderDevice 解析资源
  -> make_unique<MeshComponent>(mesh, material)
```

codec 不一定要各自成为公开类。第一版可以是注册时提供的普通函数或 lambda，等逻辑变复杂后再拆文件。

## 工厂是否需要类型擦除

字符串到不同 C++ 类型的映射天然需要擦除“具体类型”，但不需要 `std::any`、通用反射系统或复杂基类。使用返回统一基类指针的 `std::function` 就足够：

```cpp
struct SceneLoadContext {
    AssetManager& assets;
    RenderDevice& renderDevice;
    InputManager* input = nullptr;
};

using ComponentFactoryFn = std::function<
    Expected<std::unique_ptr<Component>, LoadError>(
        const rapidjson::Value&, SceneLoadContext&)>;

class ComponentFactoryRegistry {
public:
    bool Register(std::string typeName, ComponentFactoryFn factory);
    Expected<std::unique_ptr<Component>, LoadError> Create(
        std::string_view typeName,
        const rapidjson::Value& value,
        SceneLoadContext& context) const;
};
```

这里的 `std::function` 已经完成所需的类型擦除：每个 lambda 内部知道具体类型，调用方只看到 `unique_ptr<Component>`。如果当前 C++ 标准没有 `std::expected`，可以使用项目自己的 `LoadResult<T>`，包含 value、错误码、文件路径和 JSON 字段路径；不要只返回 nullptr 后向 stdout 打一句话。

GameObject 工厂同理：

```cpp
using GameObjectFactoryFn = std::function<
    Expected<std::unique_ptr<GameObject>, LoadError>(
        const rapidjson::Value&, SceneLoadContext&)>;
```

但普通场景对象不需要经过 GameObject 工厂，直接创建基础 `GameObject` 即可。只有游戏模块定义的派生对象或脚本宿主确实需要 C++ 类型时才查询 registry。

## Registry 放在哪里

建议增加：

```text
engine/src/Assets/SceneAssetLoader.h/.cpp
engine/src/Assets/SceneDesc.h
engine/src/Scene/SceneInstantiator.h/.cpp
engine/src/Scene/SceneTypeRegistry.h/.cpp
```

职责划分：

- `SceneAssetLoader`：读取 JSON、校验 schema、生成 `SceneDesc`；依赖 FileSystem，不创建 GameObject；
- `SceneDesc`：纯值类型，保存对象、组件、transform、ID 和资产引用；不包含 RapidJSON Value；
- `SceneTypeRegistry`：保存字符串到构造函数的显式映射；不拥有 Scene 或资产；
- `SceneInstantiator`：持有/接收加载上下文，根据 SceneDesc 和 registry 构造运行时对象；
- `AssetManager`：作为对外入口拥有 SceneAssetLoader，并协调场景依赖；它不应亲自解析每个组件字段。

引擎内置类型由一个明确函数注册：

```cpp
void RegisterBuiltinSceneTypes(SceneTypeRegistry& registry);
```

游戏侧类型由游戏组合根注册：

```cpp
void RegisterGameSceneTypes(SceneTypeRegistry& registry);
```

不要依赖跨翻译单元的全局静态自注册。静态库链接器可能丢弃“只有注册副作用”的目标文件，初始化顺序也不透明。Registry 应拒绝重复名称并输出清晰错误。类型名一旦写入资产就是持久格式，建议稳定、区分大小写并可使用命名空间，例如 `engine.MeshComponent`、`game.ThirdPerspectiveController`。

现有运行时 `Component::TypeId` 用于内存中的快速类别判断；场景字符串名用于持久化注册。两者职责不同，不要尝试把进程内生成的 TypeId 写入场景文件。

## 构造参数与依赖注入

现有组件已经说明工厂不能假定统一默认构造：

- `MeshComponent` 需要 Mesh 和 Material；
- `RigidBodyComponent` 需要完整 `RigidBodyDesc`；
- `AnimationComponent` 需要 Model；
- `CameraComponent` 和 `PlayerControllerComponent` 需要 InputManager；
- `SkinnedMeshComponent` 需要 Mesh、Material、SkeletonPose 等关联资源。

因此 registry 的 factory 必须接收 `SceneLoadContext`。上下文只暴露构造所需服务，不要让每个组件直接取得整个 Engine。资源型组件通过 AssetManager 加载共享资产；创建 primitive mesh 若当前必须使用 RenderDevice，则由专门的 Mesh/Primitive 服务处理，避免把裸 OpenGL 类型带进场景加载器。

长期可让 AssetManager 增加：

```cpp
LoadScene(path);
InstantiateScene(sceneAsset, targetScene);
```

其中 LoadScene 只产生已验证的资产/描述，InstantiateScene 才修改目标 Scene。这样同一场景资产可以实例化多次，也为后台解析、主线程实例化留下空间。

## 与现有 GameObject/Scene API 的衔接

工厂返回 `unique_ptr<Component>` 后，需要一条非模板的内部挂接入口。不要让 SceneAssetLoader 成为 `GameObject` 的 friend 并直接修改 `m_components`。建议：

```cpp
// 面向引擎构建流程；可设为 private 并 friend SceneInstantiator
bool GameObject::AttachComponent(std::unique_ptr<Component> component);
```

现有类已经有对应 private 实现，可让 `SceneInstantiator` 成为窄 friend，或由 Scene 提供内部 builder API。GameObject 也类似：实例化器需要把工厂产生的 `unique_ptr<GameObject>` 交给 Scene 的现有 `AttachObject` 路径，而不是重新实现 owner、parent、OnAttach 和延迟命令规则。

关键顺序是：

```text
创建 detached GameObject
  -> 写入 name/local transform/enabled
  -> 创建并配置全部 Component
  -> 建立 detached children
  -> 资源与必填引用全部成功
  -> 整棵树接入 Scene
  -> GameObject::AttachToScene 递归触发 Component::OnAttach
```

这样 `RigidBodyComponent::OnAttach()` 看见的是最终 transform 和完整 Desc，失败时也不会在目标 Scene 中遗留半棵对象树。当前 GameObject 的层级与挂接接口较封闭，实现时最好增加一个窄的 `SceneInstantiator` 访问通道，而不是把这些所有权函数全部公开给游戏代码。

如果第一版不构建 detached tree，至少也必须遵守“先完整解析和加载依赖，再开始修改 Scene”，并在实例化失败时回滚本次创建的根对象。不要解析一个字段就立刻向 Scene 添加一个对象。

## 分阶段解析与引用解析

推荐五个阶段：

1. **Parse**：JSON DOM 转为 SceneDesc，记录源文件和字段路径；
2. **Validate**：检查 version、唯一 ID、类型名、字段范围、组件唯一性和引用格式；
3. **Resolve assets**：规范化并加载 Model/Material 等依赖，任何失败都不修改目标 Scene；
4. **Instantiate**：创建全部对象与组件，建立 `id -> GameObject*` 临时表；
5. **Link/commit**：解析 main camera、对象引用和组件引用，最后把完整根对象提交给 Scene。

组件间引用不能在解析到第一个对象时立即查找，因为目标可能出现在文件后面。JSON 中保存 ID，Link 阶段统一解析。临时表只在一次实例化过程中存在；不要把裸指针写回 SceneAsset 缓存。

## 错误处理与失败原子性

建议错误至少包含：

```text
scene path
JSON path，例如 $.objects[2].components[0].material
错误类别：syntax / schema / unknown-type / missing-asset / construction
可读消息
```

语法错误应附 RapidJSON offset；schema 错误应给出 JSON path。Loader 可以收集多个独立校验错误，但资源创建失败后不应继续制造级联噪声。

加载应尽量具有失败原子性：失败时返回错误，不替换正在运行的 Scene，也不缓存半成品。最简单的加载切换方案是先构建新的 `Scene`，成功后由上层替换旧 Scene；若必须加载进已有 Scene，则以本次实例化根节点为事务边界，失败时统一销毁。

## 版本、默认值与迁移

顶层必须有 `version`。第一版只接受 version 1，不要静默猜测。以后可以在 SceneAssetLoader 中执行纯数据迁移：

```text
v1 JSON -> v1 SceneDesc
v2 JSON -> migrate -> current SceneDesc
```

组件若独立演进，可增加可选的组件 `version`，但不必一开始给每个组件都加。默认值应定义在对应 Desc/codec 中，并与运行时构造默认值保持一致；缺失字段使用默认值，类型错误不能退回默认值。

## 建议的最小实施顺序

1. 修正 scene JSON：加入 version、稳定 ID、transform、components、children/instance，移除注释和重载的 type；
2. 定义 `SceneDesc/ObjectDesc/ComponentDesc` 和结构化 `LoadError`；
3. 实现 SceneAssetLoader，只解析与验证，不创建 Scene；
4. 实现显式 SceneTypeRegistry，先注册基础 GameObject、CameraComponent、灯光和 RigidBodyComponent；
5. 增加 SceneInstantiator 的窄内部挂接入口，保证 transform/属性在 OnAttach 前完成；
6. 接入 AssetManager，支持 MeshComponent 的 primitive/material 和 Model instance；
7. 增加 ID Link 阶段，设置 main camera并支持组件对象引用；
8. 最后再考虑 Prefab、覆盖属性、热重载、编辑器保存和场景流式加载。

## 当前结论

第一，不给每个运行时类直接增加 RapidJSON `ParseProperties()`；采用资产层 codec/工厂，把完整构造参数准备好后再创建组件。第二，字符串工厂使用 `std::function -> unique_ptr<Base>` 的轻量类型擦除即可，不需要通用反射。第三，内置类型与游戏类型都显式注册，避免全局静态自注册。第四，解析、资源解析、实例化和引用链接必须分阶段，尽可能在修改目标 Scene 前发现错误。第五，模型实例、GameObject C++ 派生类型和 Component 类型必须使用不同字段表达，不能继续共用一个含义模糊的 `type`。
