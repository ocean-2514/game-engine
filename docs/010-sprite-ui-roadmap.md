# Sprite 与 UI 之前的补全路线

状态：基于 2026-10-21 的代码检查形成的**历史规划快照**，不是当前完成度清单。其中窗口、输入、屏幕视图等部分后来已有初版，表格中的“当前”仅指当时状态。2026-10-27 对新实现的审查、修正优先级与 Sprite/UI 细化方案见 [011 2D 渲染复查与 Sprite/UI 设计](011-2d-rendering-sprite-ui.md)。相关背景见 [001 渲染架构](001-rendering-architecture.md)、[003 组件架构](003-component-architecture.md)、[004 资产架构](004-asset-architecture.md) 和 [008 场景资产架构](008-scene-asset-architecture.md)。

## 先说结论：不需要先做全局事件总线

按钮点击是 UI 树内的定向事件，物理碰撞目前已有可轮询的 Scene 事件队列，Scene/Component 结构变更也有各自的安全点。这些问题的作用域、时序和所有权不同；此时加一个全局 `EventBus` 并不能解决鼠标坐标、命中测试、输入抢占、视图分层或对象销毁后的回调安全性，反而会隐藏这些边界。

初版采用三种明确通信方式：窗口回调把输入**值事件**写入 `InputManager` 的本帧队列；`UIContext` 对 UI 树做命中测试后直接向目标控件分发；按钮对所属应用暴露 `SetOnClick(std::function<void()>)` 或语义化 Action ID。物理/动画事件继续由各系统产生值事件，不为它们共建总线。只有当多个互不拥有的系统确实需要订阅同一种稳定的业务事件时，再设计带作用域、订阅令牌和明确派发安全点的类型化 dispatcher。不要用字符串类型 + `std::any` + 裸指针订阅者作为基础设施。

## 当前代码核对：哪些能复用，哪些会挡住 UI

| 现状 | 对 Sprite/UI 的影响 | 结论 |
| --- | --- | --- |
| `TextureDesc` 有 `R8`、`SRGBA8`，`SamplerDesc` 可夹取边缘；纹理加载有缓存 | 已有图片资源基础，也可承载字形覆盖率图 | 复用；字体资产另设加载器，不把字形塞进 `MaterialAssetLoader` |
| `MeshComponent`、`RenderCommand`、`RenderPhase::Transparent/Overlay`、稳定排序已有 | 可先渲染世界中的单张透明 quad | 世界 Sprite 可渐进实现；`Overlay` 不等于独立屏幕空间 UI 视图 |
| `Scene::Render()` 只接受 `CameraComponent`，一个 view 会遍历全部场景对象 | UI 不该依赖主相机、灯光或场景对象遍历 | 增加独立 Screen/UI view；不要为 UI 造假相机对象 |
| `RenderQueue::Execute()` 对每条命令设置 `uModel/uNormalMatrix/uSkinned`，每 Shader 设置 3D 相机/光照 | UI quad、文字没有法线和骨骼；3D Shader 约定不适合作为通用绘制约定 | 按 draw/view 类型分开上传，别把 UI 特例塞进默认光照材质 |
| GLFW 初始化时固定 `GLFW_CURSOR_DISABLED`；`InputManager` 保存键/鼠标当前状态和位置，滚轮回调覆盖值 | 看不见 UI 指针、无法可靠区分点击边沿/同帧按下再释放；滚动可能丢失 | 先补光标模式、本帧输入事件/边沿与滚轮累计 |
| `Window` 宽高只在初始化时记录；未找到 framebuffer-size 回调或 `glViewport` 更新 | 窗口缩放、高 DPI 时鼠标命中区域与绘制坐标可能错位 | 先区分逻辑窗口尺寸与 framebuffer 像素尺寸，建立 resize/viewport 规则 |
| `Material` 参数只有标量、`vec2/vec3`，共享资源带 revision；Mesh 顶点上传是一次性静态 `vector<float>` | UI 多控件若逐个修改共享材质会互相覆盖；文字每字一个静态 Mesh 会昂贵 | 颜色/UV/裁剪等做每条 UI 绘制数据；动态合批以后加，不在第一步重构全部 Mesh |
| 场景 JSON 已有 Codec/Registry；仓库里没有 UI/字体资产实现或相关测试目标 | 存档接入有现成路径，但格式还无稳定运行时语义 | 先跑通代码创建的 UI，再加 UI 资产/Codec；准备最小回归用例 |

## 推荐顺序与验收

### 0. 固定一套小型回归场景（与步骤 1 并行）

在测试/示例中保留当前 3D 场景，并逐步加一张世界 Sprite、一个屏幕图片、一行文字和一个按钮。记录窗口 resize、DPI、销毁控件、多个 RenderView、透明遮挡和按钮点击的预期。新增纯逻辑测试可先覆盖输入边沿、UI 矩形命中/坐标转换、布局和事件分发；图形截图或人工烟雾测试单独做。此步不要求先建大型测试框架，但后续每步都应保证原有模型、动画和场景加载仍可用。

### 1. 窗口尺寸与输入快照：真正的前置工作

建议新增 `WindowSize`（逻辑窗口宽高）、`FramebufferSize`（像素宽高）和可选 `ContentScale` 值类型；`Window::GetWindowSize()`、`GetFramebufferSize()`、`SetCursorMode(CursorMode)`，窗口后端接入 GLFW resize/framebuffer-size 回调。不要再把构造时的 `m_width/m_height` 当作永久值。画面为零尺寸（最小化）时跳过依赖宽高的投影/绘制，避免除零。

`InputManager` 保留现有轮询 API，但增加 `WasKeyPressed/Released`、`WasMouseButtonPressed/Released`，并将光标移动、按钮按下/松开、滚轮等按到达顺序写为本帧轻量 `InputEvent` 值列表；滚轮应累计而非覆盖。`Engine::Run()` 的固定顺序应是 `PollEvents → 构造/冻结本帧输入 → UI 分发 → 游戏 Update → Render → 清除本帧瞬态`。保留连续按键状态供角色控制，别把所有 gameplay 输入强行改成事件驱动。

坐标契约：GLFW 光标坐标按逻辑窗口坐标处理；UI 布局/命中也用逻辑坐标，原点左上、Y 向下。`RenderDevice` 的 viewport 使用 framebuffer 像素；由两组尺寸计算比例，不能在高 DPI 下直接拿 framebuffer 像素与鼠标逻辑坐标比较。初版只有一个窗口时无需泛化多窗口输入。

验收：普通/禁用光标模式可切换；单帧快速点击不会漏；同帧多次滚轮可累计；resize/高 DPI 后指针位置与屏幕 quad 一致。

### 2. 独立的屏幕渲染视图与最小 2D 绘制命令

保留 `Scene::Render(queue, camera, aspect)` 作为 3D 世界视图入口；增加显式的屏幕视图入口，例如 `RenderQueue::BeginScreenView(ScreenViewDesc)`。`ScreenViewDesc` 至少包含逻辑尺寸、framebuffer viewport、投影/坐标约定；初版用正交投影，可由队列/前端生成。`Application::Render` 先提交世界 Scene，再提交 UI 视图，Engine 最后仍只执行一次队列。不同 view 按提交顺序执行，UI 不参与世界视图的透明距离排序，也不依赖 `Scene::m_mainCamera`。

增加 `ScreenQuadCommand`（屏幕矩形、UV 矩形、纹理、线性色调 `vec4`、同层顺序；裁剪矩形可后补）及 `SubmitScreenQuad`，与当前世界 `RenderCommand` 分开执行路径。屏幕路径固定关闭深度测试/写入、开启 alpha blending、关闭背面裁剪；只上传屏幕投影和 quad 参数，不上传灯光、法线矩阵、骨骼。共享 UI Shader/quad 几何，颜色和 UV 属于**每次 draw**，不反复修改共享 `Material`。`RenderDevice` 增加不含 OpenGL 类型的 `SetViewport(Viewport)`；OpenGL 后端负责转换为 `glViewport`。不要为此先实现通用 RenderGraph、FBO、Vulkan 风格 Pipeline 或全套 2D batcher。

验收：resize 后 3D 与 UI 投影都正确；两个世界视图后仍能绘制一次 UI；大量同层 quad 按提交次序稳定覆盖；UI 不受场景灯光/深度影响。需要嵌套裁剪时再增加 `ScissorRect` 和后端 scissor 状态，并定义逻辑矩形到 framebuffer 像素的换算。

### 3. 先实现世界 Sprite，再实现屏幕 Image

建议 `SpriteComponent` 放在 `Scene/Components/`，只负责世界空间 XY quad + 纹理/UV + tint + 尺寸（可先沿用 GameObject 的 Transform）。它可使用现有 `RenderCommand`/透明材质/`RenderPhase`，第一版不要引入粒子、动画图集或 billboarding；说明双面可见性与 alpha 模式。与它不同，屏幕图片由 UI 模块的 `ImageElement` 产生 `ScreenQuadCommand`，不受 Camera、物理插值或世界排序支配。两者可共享纹理、采样器和 quad 绘制资源，但不能混淆坐标和生命周期。

Sprite 图像按颜色纹理加载（通常 sRGB），UV 原点/图片翻转要与现有 `ImageLoader` 约定一致。透明 PNG 不应仅凭“存在 alpha 通道”断定全部像素需要混合：opaque、masked、transparent 仍由显式表面语义决定。`SpriteComponent` 的场景 Codec 待运行时参数稳定后再加，沿用现有 parse/resolve/create，不给组件加 `ParseProperties()`。

验收：世界 Sprite 能随对象移动，并与 3D 物体按预期遮挡；屏幕 Image 永远留在 UI 坐标，改变主相机不改变其位置。

### 4. 最小 UI 树与输入路由，不先做全局总线

新增 `UIContext`（初版由 `Application` 持有，不自动混入 `Scene`）、`UIElement`（`unique_ptr` 子树、局部矩形、可见/可交互状态、父指针）以及 `ImageElement`。先只做固定位置、尺寸、锚点/简单父子相对布局；布局脏标记与屏幕 resize 触发重算。不要先做完整 Flex/Grid、CSS、约束求解器或编辑器。UI 树存储控件，渲染阶段生成值命令；RenderQueue 不持有 `UIElement*`。

`UIContext::ProcessInput(const InputFrame&)` 应从顶层到下层命中测试，只把 pointer down/up/move 送给命中的最高控件，返回 `UICapture`（至少含 pointer/keyboard 是否被 UI 使用）。可定义 `UIEvent{type, position, button}` 与 `UIElement::OnPointerEvent(...)`；控件回调期间的增删进入 UIContext 自己的延迟队列，在分发结束后提交。点击语义是同一按钮上按下并释放才激活；移出、销毁、失焦时取消 pressed/capture。游戏输入读取目前直连 `Engine::GetInputManager()`，接入 UI 后至少应让玩家控制器尊重 `UICapture`；以后再把控制器输入改为显式传入经过路由的输入视图。

验收：重叠控件只有顶层响应；控件在回调里销毁自身不崩溃；UI 点击不同时转动游戏相机；未命中 UI 时原游戏输入不受影响。

### 5. 字体与 Text，再做 Button

新增 `FontAssetLoader`、`FontFace`/`Font`、`GlyphAtlas`、`TextLayout`、`TextElement`。字体加载/字形栅格化位于 Assets；atlas 作为 `R8` 线性数据纹理交由 `RenderDevice` 上传；排版在 UI 前端处理 UTF-8 解码、字形 advance、基线、行高、换行与缺字 fallback；渲染只接收字形 quad。第一版可以先固定字体、字号和有限字库，但若示例含中文，不可用“按字节逐字符”的 ASCII 实现冒充 UTF-8 支持。明确至少一个目标字体覆盖示例字符；字体许可和打包方式也要确定。

现有 `CreateTexture(...)` 只有整张初次上传：若选择按需生成字形 atlas，需要新增后端无关的 `UpdateTextureRegion(...)` 与 atlas page/失效策略；若暂不加，先在加载时预烘焙**明确列出的字形集**，并把动态文本限制写清楚。字体栅格化库在进入此阶段时选定，避免 UI 基础层直接暴露库类型。初版每字提交 quad 可以验证正确性，性能不够再按 atlas page/裁剪区域合批。

最后新增 `ButtonElement`，组合背景 `ImageElement` 与 `TextElement`，处理 normal/hover/pressed/disabled 状态，并以 `SetOnClick` 或 UI Action ID 通知所属应用。回调不能捕获比按钮更短命的裸对象；跨 Scene 切换/异步任务应使用可验证的 handle 或应用层生命周期约束。键盘焦点、Tab 导航、IME/文本输入、无障碍、复杂排版与本地化是后续独立阶段，不应谎称初版按钮已覆盖。

验收：UTF-8 示例文本宽度/换行正确，字形缺失可见且可诊断；按钮 hover/press/click 状态准确；回调可安全请求删除控件或切换场景。

### 6. 运行时稳定后再补资产格式、批处理和扩展

UI 树如需存盘，可新增 `UIAssetLoader`/UI Codec，使用稳定类型名和 `parse → resolve → create`，不要把 UI JSON 直接反序列化进运行时对象。布局、字体、图片引用和按钮 Action ID 先定好运行时语义，再定版本化 schema。Sprite Codec 可早于 UI 资产单独接入。

性能实测出现压力后，再给屏幕 quad/字形做动态顶点缓冲与合批；批次按纹理/atlas page、裁剪矩形和顺序边界切分，不能为了减少 draw call 打乱叠放顺序。圆角、九宫格、遮罩、滚动区域、嵌套裁剪、导航/焦点、RenderTarget/后处理与全局事件系统都属于明确需求驱动的下一轮，不是 Sprite 或首个可用按钮的门槛。

## 建议文件与接口草图（按需逐步加入）

```text
Platform/Window.h               WindowSize / FramebufferSize / CursorMode / resize
Input/InputEvent.h               InputEvent / InputFrame / UICapture
Input/InputManager.h             边沿状态、本帧有序事件、滚轮累计
Renderer/RenderQueue.h           BeginScreenView / SubmitScreenQuad
Renderer/RenderDevice.h          SetViewport（字形按需更新时再加 UpdateTextureRegion）
Scene/Components/SpriteComponent.h
UI/UIContext.h                   ProcessInput / Layout / Submit
UI/UIElement.h                   子树、Rect、可见性、输入钩子
UI/ImageElement.h
Assets/FontAssetLoader.h         字体资源与栅格化边界
UI/TextLayout.h / TextElement.h
UI/ButtonElement.h               SetOnClick
```

这些是推荐的职责边界，不要求一次创建全部文件。尤其不需要在开始画第一张 Sprite 前先引入 `EventBus`、完整 ECS、通用 RenderGraph 或一整套 UI 设计器。
