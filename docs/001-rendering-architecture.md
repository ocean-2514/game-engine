# 渲染架构与图形 API 抽象

- 状态：讨论稿
- 日期：2026-08-08
- 当前后端：OpenGL 3.3 Core
- 潜在后端：Vulkan（仅作为架构兼容性假设，近期不实现）

## 背景

项目当前是一个用于学习和深入研究的 C++17/OpenGL 游戏引擎原型。现有代码已经包含：

- `Engine` 与主循环；
- `Application` 生命周期；
- GLFW 窗口及输入；
- GLAD；
- `GraphicsAPI`；
- `ShaderProgram`；
- 基础 GLSL Shader。

当前公开图形接口直接使用 `GLuint`、`GLenum` 等 OpenGL 类型。这对于早期学习很直接，但会让游戏层、Renderer 前端和 OpenGL 后端耦合。即使未来不实现 Vulkan，明确边界也能改善模块职责、资源生命周期和可测试性。

## 本次结论

渲染架构逐步分为三层：

```text
游戏与引擎功能层
Renderer / Material / Mesh / Camera
              │
              ▼
渲染硬件抽象层 RHI
RenderDevice / Buffer / Texture / Pipeline / CommandList
              │
       ┌──────┴──────┐
       ▼             ▼
  OpenGL 后端     Vulkan 后端（假设）
```

核心边界为：

> 游戏代码和 Renderer 前端不知道 OpenGL；OpenGL 后端可以充分利用 OpenGL，不必隐藏自身的实现特点。

目前不应为了一个尚未确定的 Vulkan 后端设计庞大、完备的通用 API。先用 OpenGL 实现一套小型 RHI，再通过实际渲染功能检验抽象是否合理。

## 不应采用的抽象方式

仅仅包装 OpenGL 函数并不能形成跨后端抽象。例如：

```cpp
virtual void BindShader(uint32_t id) = 0;
virtual void BindVertexArray(uint32_t id) = 0;
virtual void SetUniform(...) = 0;
```

这仍然是在公开接口中表达 OpenGL 状态机。Vulkan 没有相同的全局绑定模型，这类接口会迫使 Vulkan 后端模拟 OpenGL。

RHI 应描述渲染资源和渲染意图，而不是逐个映射 `gl*` 函数。

## 公共资源与描述类型

### OpenGL 优先，而不是预先模拟 Vulkan

本项目的首要目标是学习和实现一个质量较好的 OpenGL 引擎，并不要求开发者在尚未学习 Vulkan 时提前设计出正确的 Vulkan 抽象。

因此，当前架构采用以下约束：

1. OpenGL 是当前唯一需要完整支持的后端；
2. 可以在引擎内部明确使用 OpenGL 概念，不为假设中的后端牺牲可理解性；
3. 游戏层和 Renderer 前端尽量不直接持有 `GLuint`、`GLenum`，以控制依赖扩散；
4. RHI 初期只是 OpenGL 实现与上层渲染功能之间的边界，不宣称已经是成熟的跨 API 标准；
5. 在出现第二个真实后端之前，不为 Vulkan 独有的需求增加复杂接口；
6. 如果未来实验 Vulkan，可以根据两个真实实现重构共同接口，而不是现在依靠猜测定型。

“不暴露裸 OpenGL 类型”不等于“不能采用 OpenGL 风格设计”。当前完全可以实现清晰、好用的 OpenGL Renderer，只需把后端细节限制在明确的模块内。

### OpenGL 对象分类

OpenGL 中带有 `Object` 的类型并不都属于 Buffer。建议按职责分类：

| OpenGL 对象 | 实际职责 | 引擎中的建议概念 |
|---|---|---|
| VBO | 保存顶点数据的 Buffer Object | `Buffer`，usage 为 `Vertex` |
| EBO/IBO | 保存索引数据的 Buffer Object | `Buffer`，usage 为 `Index` |
| UBO | 保存 uniform block 数据的 Buffer Object | `Buffer`，usage 为 `Uniform` |
| SSBO | 保存 Shader 可读写结构化数据的 Buffer Object | 以后需要时增加 `Storage` usage |
| PBO | CPU/GPU 像素传输使用的 Buffer Object | 以后需要时增加 `Transfer` 或 `Staging` usage |
| Indirect Buffer | 保存间接绘制参数的 Buffer Object | 以后需要时增加 `Indirect` usage |
| VAO | 记录顶点属性格式及相关绑定状态，本身不保存顶点数据 | `VertexArray`，以后可演进为 `VertexLayout` + Pipeline 状态 |
| Texture | 采样或图像存储资源 | `Texture` 或更通用的 `Image` |
| RBO | 通常作为颜色、深度或模板附件的不可采样图像存储 | `Renderbuffer`，或作为 Render Target 的内部实现细节 |
| FBO | 组合颜色、深度和模板附件，定义渲染目标 | `Framebuffer` 或 `RenderTarget` |
| Shader Object | 单个 Shader 阶段的编译对象 | OpenGL 后端的临时编译资源 |
| Program Object | 链接后的 Shader 可执行程序 | 当前为 `ShaderProgram`，以后可并入 `GraphicsPipeline` |
| Sampler Object | 纹理过滤与寻址状态 | `Sampler` |

其中只有 VBO、EBO、UBO、SSBO、PBO 和 Indirect Buffer 属于同一类底层 OpenGL Buffer Object。VAO、RBO 和 FBO 不应被放入 `BufferUsage`。

`BufferUsage` 这个名字表达的是同一种线性 GPU 存储的用途，而不是罗列所有 OpenGL 对象：

```cpp
enum class BufferUsage {
    Vertex,
    Index,
    Uniform
};
```

当前只实现真正用到的三项。等引擎引入计算、异步上传或间接绘制后，再增加 `Storage`、`Transfer`、`Indirect` 等用途，不提前加入尚无用例的枚举。

### 当前建议的具体 OpenGL 资源

在第一阶段，可以使用以下直观对象：

```text
OpenGLDevice
├─ OpenGLBuffer
├─ OpenGLVertexArray
├─ OpenGLShaderProgram
├─ OpenGLTexture
├─ OpenGLFramebuffer
└─ OpenGLRenderbuffer（需要 MSAA 或深度附件时再引入）
```

这些类可以明确服务于 OpenGL，不需要假装每一个类都能直接映射到 Vulkan。上层 `Mesh` 持有 Buffer 和 VertexArray，上层 Render Target 持有 Texture/Renderbuffer 和 Framebuffer，但不直接操作它们的 `GLuint`。

第一版甚至可以暂不建立纯虚基类，而是使用具体的 OpenGL 类，并通过目录和头文件边界控制依赖。等第二个后端成为真实需求后，再选择：

- 为已有资源提取接口；
- 使用不透明强类型句柄；
- 或让 Renderer 前端通过 `RenderDevice` 间接访问资源。

这种“从两个真实实现中提取共同点”的方式，比先猜测 Vulkan 更可靠。

公开接口不应出现：

- `GLuint`；
- `GLenum`；
- `GL_TEXTURE_2D`；
- `GL_ARRAY_BUFFER`；
- `GL_STATIC_DRAW`。

使用引擎自己的强类型枚举和描述结构：

```cpp
enum class BufferUsage {
    Vertex,
    Index,
    Uniform
};

enum class MemoryUsage {
    Static,
    Dynamic,
    Stream
};

struct BufferDesc {
    BufferUsage usage;
    MemoryUsage memoryUsage;
    std::size_t size;
    const void* initialData = nullptr;
};
```

资源可以先采用抽象对象和 RAII：

```cpp
class Buffer {
public:
    virtual ~Buffer() = default;
};

class Texture {
public:
    virtual ~Texture() = default;
};

class Shader {
public:
    virtual ~Shader() = default;
};
```

由设备负责创建资源：

```cpp
class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual std::shared_ptr<Buffer>
        CreateBuffer(const BufferDesc& desc) = 0;

    virtual std::shared_ptr<Texture>
        CreateTexture(const TextureDesc& desc) = 0;

    virtual std::shared_ptr<Shader>
        CreateShader(const ShaderDesc& desc) = 0;
};
```

OpenGL 类型只保留在后端实现中：

```cpp
class OpenGLBuffer final : public Buffer {
public:
    ~OpenGLBuffer() override;

private:
    GLuint m_buffer = 0;
};
```

只有 `Renderer/OpenGL` 目录允许直接包含 GLAD 和使用裸 OpenGL 类型。

## Pipeline 抽象

公共接口长期应从裸 `ShaderProgram` 提升为 `GraphicsPipeline`。

```cpp
struct GraphicsPipelineDesc {
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    VertexLayout vertexLayout;
    RasterState rasterState;
    DepthState depthState;
    BlendState blendState;
};
```

OpenGL 后端可以在 Pipeline 内保存 Program 和相关固定功能状态，在绑定时应用深度、混合、剔除等设置。Vulkan 后端则可以真正创建 `VkPipeline`。

这种模型比将 `ShaderProgram` 直接作为跨后端公共资源更接近 OpenGL 与 Vulkan 的共同概念。

## 命令提交

Renderer 前端不应直接调用 `glDrawElements()`。可以先建立一个很小的命令接口：

```cpp
class CommandList {
public:
    virtual ~CommandList() = default;

    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void SetPipeline(GraphicsPipeline& pipeline) = 0;
    virtual void SetVertexBuffer(Buffer& buffer) = 0;
    virtual void SetIndexBuffer(Buffer& buffer) = 0;
    virtual void DrawIndexed(
        uint32_t indexCount,
        uint32_t firstIndex = 0) = 0;
    virtual void EndRenderPass() = 0;
};
```

OpenGL 后端初期可以立即执行命令，不必真的录制命令。Vulkan 后端将来可以把操作写入 Command Buffer。接口不应强迫两个后端采用相同的内部执行方式。

## Shader、uniform 与 Material

当前 `ShaderProgram::setFloat()`、`setVec3()` 等接口适合学习 OpenGL，但属于 OpenGL 后端能力，不应成为最终的跨 API 材质接口。

主要原因：

- Vulkan 通常使用 uniform buffer、push constant 和 descriptor set；
- 按字符串逐项设置 uniform 是 OpenGL 风格；
- 字符串查询和参数类型错误只能在运行时发现；
- 参数布局难以统一管理和验证。

建议分阶段处理：

1. 保留当前 `ShaderProgram`，后续将其移动并命名为 `OpenGLShaderProgram`；
2. 在 Renderer 前端引入 `Material`；
3. Material 收集参数，后端在提交时转换成对应的 GPU 参数机制。

```cpp
material.SetFloat("roughness", 0.5f);
material.SetTexture("albedoMap", texture);
```

对应关系：

```text
Material 参数
   ├─ OpenGL：uniform / UBO
   └─ Vulkan：descriptor set / push constant / UBO
```

因此，`ShaderProgram::setFloat()` 与 `Material::SetFloat()` 虽然形式相似，但属于不同层次。

## Window 与图形设备

当前 `Engine::Init()` 同时负责 GLFW、窗口、OpenGL Context、GLAD 和 Application。若引入其他图形 API，这些职责会发生冲突。

目标结构：

```text
Engine
 ├─ Window
 ├─ InputSystem
 ├─ RenderDevice
 └─ Application
```

配置可以逐步演进为：

```cpp
enum class GraphicsBackend {
    OpenGL,
    Vulkan
};

struct EngineConfig {
    int width = 1280;
    int height = 720;
    GraphicsBackend graphicsBackend = GraphicsBackend::OpenGL;
};
```

初始化流程：

```text
创建 Window
    │
    ▼
根据 GraphicsBackend 创建 RenderDevice
    ├─ OpenGLDevice：创建 Context 并加载 GLAD
    └─ VulkanDevice：创建 Instance、Surface、Device 和 Swapchain
    │
    ▼
初始化 Renderer
    │
    ▼
初始化 Application
```

GLFW 可以继续作为窗口实现，但 OpenGL Context 配置不应长期固定在 `Engine` 内部。

## 建议目录结构

```text
engine/src
├─ Core
│  ├─ Engine
│  ├─ Application
│  ├─ Log
│  └─ Assert
├─ Platform
│  └─ GLFW
│     ├─ GLFWWindow
│     └─ GLFWInput
├─ Renderer
│  ├─ RenderDevice
│  ├─ CommandList
│  ├─ Buffer
│  ├─ Texture
│  ├─ GraphicsPipeline
│  ├─ Material
│  ├─ Mesh
│  └─ OpenGL
│     ├─ OpenGLDevice
│     ├─ OpenGLBuffer
│     ├─ OpenGLTexture
│     ├─ OpenGLPipeline
│     └─ OpenGLShader
└─ Scene
   ├─ Camera
   └─ Transform
```

这是演进目标，不要求立即创建所有空目录和空类型。

## 渐进实施路线

1. 封装 `Window`，把 GLFW 窗口生命周期从 `Engine` 中移出；
2. 引入最小 `RenderDevice`，实现 `OpenGLDevice`；
3. 将现有 `ShaderProgram` 移入 OpenGL 后端；
4. 抽象 `Buffer`，实现 Vertex Buffer 和 Index Buffer；
5. 定义 `VertexLayout`；
6. 引入 `GraphicsPipeline`；
7. 引入最小 `CommandList`；
8. 在 Renderer 前端增加 `Mesh` 和 `Material`；
9. 实现一个上层代码完全不接触 OpenGL 类型的三角形或立方体示例；
10. 根据实际使用情况复查接口，再考虑实验性 Vulkan 后端。

## 已完成的相关基础修正

本次讨论前后已经完成：

- 补齐 `ShaderProgram` 的 uniform setter；
- uniform location 缓存支持在逻辑常量查询中更新；
- Shader 编译失败时释放 Shader；
- Program 链接失败时释放 Program；
- Program 链接结束后释放各阶段 Shader；
- 初始化失败时回收 Application、窗口和 GLFW；
- CMake 源文件由 `GLOB_RECURSE` 改为显式列表；
- Debug 构建验证通过。

## 尚未决定的问题

以下内容应在相关功能真正需要时再决定：

- 公共资源使用虚基类、强类型句柄，还是两者结合；
- CommandList 是立即执行、录制执行，还是同时支持；
- Shader 源码采用 GLSL、SPIR-V，还是建立离线编译流程；
- Descriptor/资源绑定模型如何表达；
- 是否需要 Render Graph；
- 是否引入 ECS，以及 Scene 与 Renderer 的数据边界。

这些问题不应在缺少实际用例时过早定型。

## 2026-08-08：第一阶段架构落地

本次重构完成了渐进路线中目前有真实用例支撑的部分：

```text
Engine / Game
├─ Platform::Window
│  └─ Platform::GLFWWindow
└─ Renderer::RenderDevice
   ├─ Renderer::ShaderProgram
   └─ Renderer::OpenGL
      ├─ OpenGLRenderDevice
      └─ OpenGLShaderProgram
```

具体边界：

- `Engine` 不再直接初始化或调用 GLFW、GLAD；
- `Game` 不再包含 GLFW 或 GLAD 头文件；
- `Window` 负责窗口事件、交换缓冲和平台图形函数地址查询；
- `GLFWWindow` 负责 GLFW 生命周期、OpenGL Context 创建和 GLFW 输入回调；
- `RenderDevice` 是当前最小图形设备前端，负责初始化和创建 Shader Program；
- `OpenGLRenderDevice` 负责 GLAD、Shader 编译和 Program 链接；
- `ShaderProgram` 公共接口不包含 `GLuint`、`GLenum`；
- `OpenGLShaderProgram` 保存 OpenGL Program、uniform location 缓存并执行 uniform 更新；
- 输入增加引擎级 `Key`，示例程序不再使用 `GLFW_KEY_A`；
- 销毁顺序固定为 Application、RenderDevice、Window，保证 GPU 资源在 OpenGL Context 存活期间释放。

当前代码约束已经验证：

- `GLuint`、`GLenum` 和 GLAD 只存在于 `Renderer/OpenGL`；
- `GLFWwindow` 和 GLFW 头文件只存在于 `Platform/GLFW`；
- 旧 `GraphicsAPI` 已移除；
- CMake clean build 通过。

在该阶段，路线中的 Buffer、VertexLayout、GraphicsPipeline 和 CommandList 尚未创建，因为当时项目还没有顶点数据或 Draw 调用。后续应随真实渲染用例逐步引入，并继续坚持 OpenGL 优先原则。

## 2026-08-09：Mesh 创建职责

Mesh 创建分为两层，避免把预制几何和 GPU 后端资源创建混在一起：

```text
MeshFactory（未来）
├─ CreateCube
├─ CreateSphere
└─ CreatePlane
        │ 生成 VertexLayout、顶点和索引
        ▼
RenderDevice::CreateMesh
        │ 创建当前图形后端资源
        ▼
OpenGLMesh（当前后端）
```

`RenderDevice::CreateMesh` 是所有通用 Mesh 数据进入图形后端的唯一入口。它不负责理解立方体、球体等形状，只负责把已经生成的布局、顶点和索引转换为 GPU 资源。

预制几何以后可以放在独立 `MeshFactory` 中。工厂负责几何算法和默认布局，然后调用 `RenderDevice::CreateMesh`，不直接实例化 `OpenGLMesh`。因此，更换后端不会影响立方体或球体的生成代码。

当前 `VertexLayout` 使用引擎级 `VertexDataType`，由 OpenGL 后端转换为 `GLenum`。`VertexElement::integer` 表示 Shader 输入是否为真正的整数属性；它与底层数据是否采用整数存储不是同一概念：

- `integer == true`：使用 `glVertexAttribIPointer`；
- `integer == false`：使用 `glVertexAttribPointer`，可由 `normalized` 控制归一化转换。

## 2026-08-10：RenderQueue 与跨帧状态缓存

普通 `RenderCommand` 强制包含有效的 Mesh 和 Material。Shader 不作为独立字段重复存入命令，而是由 Material 唯一确定。即使一个简单三角形没有任何材质参数，也应使用一个只持有 ShaderProgram 的空参数 Material，避免命令依赖此前遗留的 OpenGL Shader 状态。

RenderQueue 将 Material 的操作拆为：

```text
Shader 变化时：ShaderProgram::Bind
该 Shader 中的 Material 状态失效时：Material::ApplyParameters
每条命令：Mesh::Bind + Mesh::Draw
```

OpenGL uniform 值属于 Program，并且可以跨 Draw 和跨帧保留。RenderQueue 因此为每个 Shader 缓存“最后写入该 Program 的 Material 和 Material revision”：

- 同一 Material 使用同一 Shader 且参数未变：不重复上传；
- 不同 Material 使用不同 Shader：第一帧上传后，后续帧只需切换 Shader；
- 不同 Material 共享同一 Shader：切换 Material 时必须重新上传，因为它们覆盖同一个 Program 的 uniform；
- Material 参数或 Shader 改变：revision 变化，下一次执行时重新上传。

Material 在 Submit 后到 Execute 前必须保持不变。队列记录提交时 revision，如果执行时发现 Material 已改变，会跳过该命令，从而避免延迟队列中的早期命令意外看到后来修改的参数。

状态缓存假定正常渲染状态都通过 RenderQueue 控制。如果外部直接调用 ShaderProgram/Material Bind 或直接修改图形状态，必须调用 `RenderQueue::InvalidateStateCache()`。`Clear()` 只丢弃待执行命令，不清除可跨帧复用的状态缓存。

## 2026-08-10：Clear 与固定功能渲染状态

清除颜色、深度和模板附件属于渲染设备命令，不属于 Window。Window 只负责平台窗口、事件、图形函数地址和交换缓冲。

当前尚无 RenderPass/CommandList，因此暂时由 `RenderDevice::Clear(const ClearDesc&)` 提供立即执行接口：

```text
ClearDesc
├─ ClearBuffer 位标志：Color / Depth / Stencil
├─ ClearColor
├─ depth
└─ stencil
```

OpenGL 后端映射为 `glClearColor`、`glClearDepth`、`glClearStencil` 和 `glClear`。未来引入 RenderPass 后，ClearDesc 应演进为 attachment 的 load/clear 配置；Vulkan 通常在开始 Render Pass 或 Dynamic Rendering 时声明清除值，而不是模拟持久的 `glClearColor` 状态。

`glEnable` 本身不应被抽象成通用的 `Enable(Capability)`。深度、模板、剔除和混合应按职责形成强类型状态描述：

```text
DepthStencilState
├─ depthTestEnable
├─ depthWriteEnable
├─ depthCompareOp
└─ front/back StencilState

RasterizerState
├─ cullMode
├─ frontFace
└─ polygonMode

BlendState（通常每个颜色附件一份）
├─ blendEnable
├─ source/destination factor
├─ blend operation
└─ color write mask
```

这些状态最终组成 `GraphicsPipelineDesc`。OpenGL 后端在绑定 Pipeline 时通过 `glEnable/glDisable`、`glDepthFunc`、`glDepthMask`、`glStencil*`、`glCullFace`、`glFrontFace` 和 `glBlend*` 应用并缓存差异；Vulkan 后端则主要把它们写入 Graphics Pipeline 创建信息。Viewport、scissor、stencil reference 等可单独作为动态状态处理。

当前建议暂缓实现完整 Pipeline，只保留上述目标设计。等项目第一次真正需要深度测试和背面剔除时，先添加最小 `DepthStencilState` 与 `RasterizerState`，随后让 Material 从持有 ShaderProgram 演进为持有 GraphicsPipeline。不要在 RenderDevice 上增加大量 `SetDepthTest`、`SetCullFace` 形式的零散永久状态接口。

## 2026-08-23：当前残余问题
- glClear(GL_STENCIL_BUFFER_BIT) 会受到 glStencilMask 影响。
- 清除颜色附件会受到 glColorMask 影响。
- 开启 Scissor Test 时，Clear 只清除裁剪区域。
因此长期最好让 OpenGL 的 Clear 实现临时设置完整的清除写入掩码，清除后恢复；未来 RenderPass 则在 Pass 开始时统一处理这些状态。

## 2026-08-23：RenderPhase、SurfaceMode 与 RenderState

固定功能状态已经以引擎类型实现，并形成三层语义：

```text
SurfaceMode：普通材质的高层分类
    ↓ 生成一致默认值
RenderPhase：决定调度阶段和排序
RenderState：决定 GPU 的最终深度、混合和光栅化行为
```

SurfaceMode 当前包括 Opaque、Masked 和 Transparent。默认映射为：

| SurfaceMode | RenderPhase | Blend | Depth write |
|---|---|---:|---:|
| Opaque | Opaque | 关闭 | 开启 |
| Masked | AlphaTest | 关闭 | 开启 |
| Transparent | Transparent | 开启 | 关闭 |

`Material::SetSurfaceMode()` 更新上述相关默认值，但保留模板已有的 depth test/compare 和 rasterizer 配置。高级调用者仍可随后直接覆盖 RenderState 或 RenderPhase；RenderState 始终是最终执行依据，RenderQueue 不会暗中根据 Phase 改写 GPU 状态。因此 Phase 与 State 可以形成不常见组合，但调用者需要明确承担其语义。

Material 同时保存默认 RenderPhase 与默认 renderOrder。MeshComponent 默认从 Material 取得这两个值，并提供实例级 order/phase override 及清除 override 的接口。这使模型批量实例化时无需逐 Mesh 配置，同时允许单个对象调整调度顺序。

每个 RenderView 内按以下键稳定排序：

```text
RenderPhase
→ renderOrder
→ Transparent 的 view-space 深度（远到近）
→ 原提交顺序
```

当前透明距离使用对象局部原点变换后的 view-space Z。大型 Mesh 或原点偏移明显时可能不准确；引入 Bounds 后应改用世界包围盒中心。

RenderQueue 在 Execute 时接收 RenderDevice 引用。第一次 Draw 和 `InvalidateStateCache()` 后的第一次 Draw 会强制应用完整 RenderState，之后仅提交变化的 Depth、Blend、Rasterizer 子状态。OpenGL 后端负责映射为 `glEnable/glDisable`、`glDepthMask/glDepthFunc`、`glBlendFunc/glBlendEquation`、`glCullFace/glFrontFace`。CullMode::None 映射为关闭面剔除，而不是 GL_FRONT_AND_BACK。

当前 RenderState 尚未包含 StencilState、独立颜色/alpha 混合因子、color write mask 和 polygon mode。出现模板描边等真实多阶段需求后，应先增加 DepthStencilState 和有序 RenderPass，再逐步演进为 GraphicsPipeline；不要用普通物体 renderOrder 模拟完整 RenderPass。

## 2026-08-13：基于 RenderView 的渲染流程

场景更新、渲染命令生成和命令执行已经拆分：

```text
Engine::Run
├─ Application::Update(deltaTime)
├─ Application::Render(RenderQueue&)
│  └─ Scene::Render(queue, camera/aspect)
│     ├─ RenderQueue::BeginView(CameraData)
│     ├─ GameObject::RenderTree(queue)
│     │  └─ Component::OnRender(queue)
│     │     └─ RenderQueue::Submit(RenderCommand)
│     └─ RenderQueue::EndView()
└─ RenderQueue::Execute()
```

`Engine` 只驱动流程，不持有主 Scene，也不查询 Camera。具体 Application 决定本帧渲染哪些 Scene 和视角；Scene 把 CameraComponent 转换成纯渲染数据后建立 RenderView；RenderQueue 只理解 CameraData、Material、Mesh 和绘制命令，不依赖 Scene 或 CameraComponent。

当前数据按更新频率分层：

| 数据 | 作用域 | 当前载体 |
|---|---|---|
| view、projection | 每个视图 | `CameraData` / `RenderView` |
| 材质参数 | 每个 Material 状态 | `Material` 与 revision 缓存 |
| model | 每个绘制 | `RenderCommand` |

相机数据不复制到每条 RenderCommand。命令提交到当前打开的 RenderView，因此其视图归属由批次隐式且唯一地确定。`BeginView()` 拒绝嵌套视图；没有活动视图时 `Submit()` 拒绝命令；`EndView()` 关闭当前批次；`Execute()` 按视图提交顺序执行并在结束后清空本帧视图。

同一 RenderView 内，每个 Shader 只上传一次 view/projection；不同 RenderView 即使复用同一 Shader，也必须重新上传相机数据。Material 缓存仍可跨视图、跨帧保留，但相机矩阵不能使用跨帧指针缓存，因为 RenderView 在队列 Clear 后销毁且相机可以逐帧移动。

`uModel`、`uView`、`uProjection` 是当前引擎保留 uniform。Material 不应写入这些名称。以后若参数增多，可以把 CameraData 改为 UBO 或后端参数块，而不改变 Scene 和 RenderView 的前端语义。

### 多视图与可见性

当前每个 RenderView 都遍历 Scene 的对象树并提交其中的可渲染组件。这是正确、可接受的基础实现：对象不属于某个具体相机，每个视图应独立决定自己的可见集合。

不应给 GameObject 或每条 RenderCommand 添加 Camera 指针。未来筛选采用“对象提供属性，视图提供条件”的方向：

```text
GameObject / Renderer 属性       RenderView 条件
├─ visible                      ├─ visibleLayers
├─ renderLayer                  ├─ frustum
└─ worldBounds                  └─ viewport / render target
              └────匹配与剔除────┘
```

可能的渐进优化：

1. 增加 visible 和 render layer/mask，先做低成本逻辑筛选；
2. MeshComponent 或独立 Renderer 数据提供 local/world bounds；
3. 每个视图用 frustum 做视锥剔除；
4. Scene 每帧收集紧凑的 renderable/proxy 列表，视图不再遍历无渲染组件的 GameObject；
5. 大场景确有性能需求后，再引入 BVH、Octree、网格分区或 GPU culling；
6. RenderView 以后可扩展 viewport、RenderTarget、ClearSettings、LayerMask 和后处理配置。

这些优化减少每个视图的候选集合，但不改变“每个 View 独立筛选”的基本模型。当前不实现 Render Graph、并行命令生成或空间索引。

## 2026-08-16：Texture 当前边界与后续设计

### 当前实现

Texture 已按前端资源与 OpenGL 后端实现分层：

```text
游戏 / Material
└─ shared_ptr<Texture>
   └─ RenderDevice::CreateTexture(path)
      └─ OpenGLTexture
         ├─ stb_image 解码
         ├─ OpenGL 像素上传与 mipmap 生成
         └─ GLuint 生命周期管理
```

当前边界约定：

- `Texture` 只公开尺寸与 `IsValid()`，不公开 `GLuint` 或伪装成通用 ID 的 `uint32_t`；
- `OpenGLTexture` 独占 OpenGL Texture Object，并负责 RAII 删除；
- 创建失败由 `RenderDevice::CreateTexture()` 返回 nullptr 表达；
- OpenGL 后端支持 1/2/3/4 通道 8-bit 图片，并处理上传行对齐；
- Material 使用 shared_ptr 持有 Texture，纹理生命周期至少覆盖所有引用它的延迟 RenderCommand；
- Material 把每张纹理映射到确定的 texture unit，ShaderProgram 接收纹理、uniform 名称和 unit；
- OpenGL texture unit 绑定是上下文全局状态，不属于 Shader Program。RenderQueue 除每 Shader 的普通 uniform 缓存外，还跟踪最后实际绑定纹理的 Material；
- 运行期创建 Texture 时，OpenGL 后端必须恢复其临时修改的 active texture、texture binding 和 pixel unpack alignment，避免破坏 RenderQueue 状态缓存。

当前 `SetTexture(name, nullptr)` 表示从 Material 移除该纹理项。Material 无法从 Shader 自动判断 sampler 是否必需，因此“缺少 Shader 必需纹理”暂时仍由调用方负责。

### TextureDesc：描述图像资源

路径加载接口适合当前示例，但不能覆盖程序生成纹理、Render Target、HDR、深度纹理和数组纹理。出现这些需求后，应增加引擎级描述结构，而不是向公共接口加入 GLenum：

```cpp
enum class TextureDimension {
    Texture2D,
    TextureCube
};

enum class TextureFormat {
    R8,
    RG8,
    RGB8,
    RGBA8,
    SRGB8,
    SRGBA8
    // 按实际 HDR、depth/stencil 用例继续增加
};

struct TextureDesc {
    TextureDimension dimension = TextureDimension::Texture2D;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mipLevels = 1;
    bool generateMipmaps = false;
};
```

未来 `RenderDevice` 可同时提供：

```cpp
CreateTexture(const TextureDesc& desc, const ImageData* initialData);
CreateTextureFromFile(path, const TextureLoadOptions& options);
```

文件路径、翻转、色彩空间等属于加载策略；GPU format、尺寸和 mip 层级属于资源描述，两者不应永久混为一个构造函数。

### ImageData：解码与 GPU 上传分离

目前 stb_image 位于 OpenGLTexture.cpp，足以支持同步学习用例，但把文件 IO、图片解码和 GPU 创建绑定在了 OpenGL 后端。后续建议形成：

```text
Asset / IO 层
└─ DecodeImage(path, options) -> ImageData
                                  │
Renderer / RenderDevice           │ pixels + metadata
└─ CreateTexture(desc, ImageData) ◄┘
   └─ OpenGL：glTexImage*
   └─ Vulkan：staging upload（未来假设）
```

`ImageData` 可包含 width、height、channel/format、row pitch 和拥有的像素内存。这样程序生成图像、测试数据和其他解码器都可以走同一上传入口，也便于以后把 CPU 解码放到工作线程。

`stbi_set_flip_vertically_on_load()` 是库级全局状态。开始并行加载前，应改为每次加载的显式选项，并在解码结果上独立翻转，避免线程和资源之间互相影响。

### Texture 与 Sampler 分离

当前 wrap/filter 固定写在 OpenGLTexture 创建中。长期应区分：

- Texture：图像内容、尺寸、格式、mip 数据；
- Sampler：寻址、过滤、LOD 和各向异性规则。

```cpp
enum class AddressMode { Repeat, ClampToEdge, MirroredRepeat };
enum class FilterMode { Nearest, Linear };

struct SamplerDesc {
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    bool useMipmaps = true;
    float maxAnisotropy = 1.0f;
};
```

OpenGL 可使用 Sampler Object，Vulkan 则对应 VkSampler。只有当同一 Texture 需要多种采样方式，或材质开始需要可配置过滤时再实现，不为当前单一用例提前增加对象。

### 色彩空间

Texture format 必须区分颜色数据与数值数据：

- albedo、base color、UI 颜色等通常按 sRGB 解码；
- normal、roughness、metallic、AO、height 等数据纹理保持线性；
- HDR 和 Render Target 根据真实管线选择浮点格式。

当前上传统一使用线性 R8/RG8/RGB8/RGBA8。实现 sRGB 前，还需要统一 framebuffer sRGB、Shader 中的光照计算空间和最终输出转换，不能只把内部格式孤立地改成 GL_SRGB8。

### 默认资源与失败策略

未来可以由 Renderer 或 AssetManager 创建并长期持有少量默认纹理：

- 1×1 白色：缺省 albedo；
- 1×1 黑色：缺省 emissive/遮罩；
- 平坦法线 `(0.5, 0.5, 1.0)`：缺省 normal map；
- 棋盘格：资源加载失败的可视化提示。

是否使用 fallback 应由资源/材质策略决定。底层 RenderDevice 仍应明确报告创建失败，不应静默把所有失败替换成默认纹理。

### 资源缓存与异步加载

相同路径当前会重复解码并创建多个 GPU Texture。资源数量开始增长后，可引入 AssetManager/ResourceManager：

```text
规范化资源路径 + 加载选项
              │ cache key
              ▼
weak/shared Texture cache
```

缓存键必须包含会改变资源结果的选项，例如 sRGB、mipmap、目标 format，而不能只使用文件路径。

异步加载应把 CPU 文件读取/解码与 GPU 上传分成两个阶段。OpenGL GPU 创建通常仍提交到持有当前 Context 的渲染线程；不要直接从任意工作线程调用 OpenGL。

### 绑定模型的后续演进

当前 `ShaderProgram::SetTexture(name, texture, unit)` 是可工作的过渡接口，unit 可理解为资源绑定槽，而非公开的 GLuint。后续 Material/Pipeline 资源绑定成熟后，应让 Material 描述纹理参数，由后端绑定层分配或映射槽位，游戏代码不直接管理 texture unit。

可能的演进顺序：

1. 保持当前 Texture2D + Material 路径，完善错误日志和默认纹理；
2. 实现 TextureDesc、TextureLoadOptions 和 ImageData；
3. 增加 sRGB/线性格式选择及统一颜色空间；
4. 在确有多种采样方式时引入 SamplerDesc/Sampler；
5. 增加 Cubemap、Render Target/depth Texture 和必要的 usage 标志；
6. 引入资源缓存，再根据加载卡顿需求拆分异步解码与渲染线程上传；
7. Pipeline/参数布局成熟后，把字符串 sampler uniform 和 texture unit 分配收进后端绑定层。

暂不实现 bindless texture、纹理流送、虚拟纹理、稀疏纹理或通用 descriptor 系统。这些设计应由真实规模和后端需求推动。
