# 2D 视图复查与 Sprite/UI 实施设计

日期：2026-10-27。本文最初依据当日仓库代码审查；下列“审查问题”保留当时快照。随后已修复 1–4 的缓存/资源边界及 5–9 的当前版本必需部分：世界 viewport 每帧设置，窗口尺寸与输入同帧事件更新，view/command 校验，2D 有符号旋转；并将 `SpriteTest` 提升为引擎 `SpriteComponent`。多种 view 任意交错仍属未来需求，当前明确固定为世界 view 先、screen view 后。UI 的下一阶段详见 [实施草案](../drafts/ui-system-implementation-plan.md)。[010](010-sprite-ui-roadmap.md) 是此前的规划快照。

## 当前已具备的基础与边界

- `ScreenViewDesc` 有逻辑尺寸、原点和 framebuffer viewport；`Scene::Render2D` 遍历 `OnRender2D`；`RenderQueue` 有 `ScreenQuadCommand`、正交投影、共享 quad mesh 与默认 screen shader。这已经足够验证单张 2D 图片。
- `ScreenQuadCommand` 的 `shader` **并非没有使用**：`ExecuteScreenQuad` 在它非空时使用自定义 shader，否则使用 `m_defaultScreenShader`。但当前只认 `uModel`、`uColor`、`uProjection`、`uLowerLeftUV`、`uUpperRightUV`、`uTexture` 这一套固定 uniform；任意自定义 shader 的其他参数没有传递渠道。
- `Window` 增加了光标模式、窗口与 framebuffer 尺寸接口，`InputManager` 增加了按键边沿和事件列表；它们还没有完全满足 UI 的同帧输入、resize/高 DPI 坐标契约。
- `Scene::Render2D` 适合先做**属于场景的屏幕 2D 内容**。UI 的布局、命中、焦点和捕获需要自己的所有权与分发树；不建议把按钮作为仅有 `OnRender2D` 的普通 `GameObject` 组件直接铺满 Scene。

## 审查问题：先修哪些

### P0：正确性与生命周期

1. **旧屏幕命令跨帧累积、重画并持有资源。** `RenderQueue::Clear()` 清了 `m_renderViews`，却没清 `m_screenQuadViews`。`Execute()` 每帧仍遍历全部旧屏幕 view，随着帧数增加 draw call 和资源引用持续增长。应同时清两类 view、重置两种 current view，且保证异常/提前返回也能结束或清理本帧队列。验证连续两帧：第二帧不提交 screen 命令时没有旧图片。
2. **屏幕绘制破坏 3D 状态缓存。** `ExecuteScreenQuad()` 直接调用 `device.SetDepthState`，末尾强制恢复为 `depthTestEnable=true`，没有经过 `ApplyRenderState`，也没有处理 blend、depth write、cull。`m_currentRenderState` 因此可能与实际 GL 状态不一致；下一帧 3D 认为无需设置而沿用错误状态。应定义完整的 screen `RenderState`（深度测试/写入关、普通 alpha blend 开、剔除关），通过同一个状态应用路径切换；执行结束后保留准确缓存或显式失效，**不要猜测之前状态并写死恢复值**。
3. **shader/纹理缓存跨 pass 也失效。** screen 路径直接 `shader->Bind()` 和 `SetTexture(..., 0)`，未同步 `m_currentShader`、`m_currentTextureMaterial` 与 shader/material 缓存。下一帧 3D 可能跳过本应执行的 3D shader 绑定或纹理重绑。最小修正是在 screen pass 后调用 `InvalidateStateCache()`（并确保下一次 3D 会重设完整状态）；更细的优化则是在 screen 路径统一更新所有相关缓存。验证 3D→UI→下一帧 3D，尤其同一个 3D Material 连续使用的情形。
4. **初始化与销毁未成对。** `RenderQueue::Init()` 只检查 `m_quadMesh`，即使默认 shader 编译/链接失败也返回成功，之后空 shader 会被解引用；还应检查 mesh 的 `IsValid()` 与 shader。新持有的 GL mesh/shader 没有在 `Engine::Destroy()` 销毁窗口/GL context 前显式释放；`Clear()`/`InvalidateStateCache()` 不释放这两个成员。建议 `RenderQueue::Shutdown()` 在 context 仍有效时清命令、缓存与默认 GPU 资源，Engine 的失败分支及正常销毁路径均调用；重复调用安全。

### P1：视图、窗口与输入契约

5. **viewport 只有 screen pass 设置。** 当前 `Execute()` 总是先跑所有 3D view、再跑所有 screen view。若最后一个 screen view 是子区域，下一帧 3D 沿用其 viewport；GLFW resize 回调不一定在每帧触发。3D view 也需要明确目标 viewport（或执行前设置全 framebuffer viewport）。若保持“世界始终先于 UI”的语义应在 API 中明说；若以后需要 `World A → UI A → World B → UI B`，需要单一有序 view 列表，不能永远分两个容器执行。
6. **窗口尺寸接口目前不可靠。** `GLFWWindow::Init()` 只给 `m_windowWidth/Height` 赋值；未看到初始 `m_frameBufferWidth/Height` 的赋值，resize 回调也只调用 `glViewport`，没更新四个尺寸字段。读取 framebuffer 尺寸可能得到未初始化值，窗口改变大小后逻辑尺寸仍是旧值。建议 GLFW 后端在创建后查询两套尺寸，分别注册 window-size 与 framebuffer-size 回调更新字段；`Window` 不直接调用 OpenGL，viewport 由 `RenderDevice` 在执行 view 时设置。逻辑宽高为零时跳过投影/绘制。示例 `Game::Render()` 当前用逻辑尺寸构造像素 `viewport`，高 DPI 下需改为 framebuffer 尺寸和显式换算。
7. **输入事件延后一帧。** `Engine::Run()` 在 `PollEvents()` 后读取 `GetEvents()`，但 `InputManager::ClearState()` 到帧末才把 `m_pendingEvents` 移给 `m_events`；故 UI 若使用 `GetEvents()` 会在下一帧收到刚发生的点击，而 `WasMouseButtonPressed` 等边沿却是当前帧。建议在 `PollEvents()` 后调用 `BeginFrame/FinalizeEvents()` 冻结本帧队列，Update/UI 都读取同一帧快照；帧末只清瞬态。滚轮聚合值目前仍是覆盖赋值，同帧多次滚动应累加；有序事件保留原始各次滚动。
8. **View/Command 校验和 shader 合约不足。** `BeginScreenView()` 未校验有限且正的逻辑宽高与有效 viewport；零尺寸会送入 `glm::ortho`。它和 `BeginView()` 也只分别检查自己的 current view，因此可同时打开两种 view，提交顺序语义不清。`IsValid(ScreenQuadCommand)` 只查纹理，未检查自定义 shader、矩阵、UV/颜色的有限值；默认 shader 也未在提交或执行前保证有效。至少先校验尺寸、纹理/有效 shader，拒绝同时打开两种 view，空帧和最小化窗口直接跳过。自定义 shader 的固定 uniform 约定需要写进接口注释或使用独立的 screen effect 类型，避免调用者传入普通 3D shader 却静默只画出错误结果。
9. **2D 旋转丢失符号。** `GameObject::GetRotation2D()` 和 `GetLocalTransform2D()` 使用 `glm::angle(quat)`，该角度只表示幅值；`SetRotation2D(-θ)` 后渲染很可能仍按 `+θ`。应从变换后的 X 轴或四元数 Z 轴分量用 `atan2` 得到有符号角，并验证父子旋转、负角与带 3D 旋转的对象如何投影到 2D。若 UI 将来有独立 Transform2D，别再隐式取 3D quaternion 的幅值。

这些问题中 1–4 建议在正式 Sprite/UI 前修复。5–7 是可交互 UI 前置；8–9 与 2D 组件一起补。当前示例运行成功只说明单帧/单 view 的基本路径可用，不能覆盖跨帧缓存、resize 或销毁次序。

## 问题 1：ScreenQuadCommand 的 shader 与“2D Material”

第一版可以**保留可选 shader**，但它的语义必须明确为“使用固定 screen-quad uniform 合约的效果替换”，不是任意材质。`nullptr` 表示默认图像 shader。投影属于 view；矩阵、tint、UV 和纹理属于每条 draw command；shader 可跨许多 quad 共享。这里不需要为每张图片复制一个普通 `Material`，也不应通过修改共享 Material 的 tint/UV 来提交多个 UI 控件，因为延迟队列最终执行时可能只看到最后一次修改，现有 revision 检查还可能跳过命令。

后续出现字体（R8 coverage）、九宫格、灰度、遮罩等**多个可复用效果**时，再把 `shader` 从命令提升为 `ScreenMaterial`/`ScreenEffect`：保存 shader、静态参数和 2D render state；每条 `ScreenQuadCommand` 只带 effect 引用和变化的数据（texture、transform/rect、UV、tint、order、必要的 clip）。例如：

```cpp
struct ScreenMaterial {  // 概念草图，不是已实现 API
    std::shared_ptr<ShaderProgram> shader;
    RenderState state;             // 不含逐控件的 tint/UV/position
    // 少量效果级参数；需要时再确定类型与绑定方式
};
struct ScreenQuadCommand {
    std::shared_ptr<const ScreenMaterial> effect; // 空 => 默认 image effect
    std::shared_ptr<Texture> texture;
    glm::mat4 modelMatrix;
    glm::vec2 uvMin, uvMax;
    glm::vec4 tint;
    int32_t renderOrder;
};
```

若偏好复用现有 `Material`，也可以先把它抽象成与 3D mesh 无关的 shader+state+静态参数容器，再把 `uModel/uProjection/tint/UV` 始终保留为 draw/view 参数；但当前 `Material::ApplyParameters()`、材质 revision、3D RenderQueue 缓存与 screen 的直接 uniform 上传是两套路径，贸然共用类会把缓存错误带入 UI。建议先保持**一个明确的内建 screen shader + 可选固定合约 shader**，等第二种真实效果出现时再引入最小 `ScreenMaterial`，不预先复制一整套 3D Material 系统。

## 问题 2：从当前实现继续做 Sprite/UI

### 先区分三个用途

| 用途 | 所有权/坐标 | 提交路径 | 第一版建议 |
| --- | --- | --- | --- |
| 场景 2D Sprite | `GameObject` 的 2D Transform，逻辑屏幕坐标 | `Scene::Render2D` → `OnRender2D` → screen quad | 将 `SpriteTest` 整理为 `SpriteComponent`，可用于场景背景、简单 HUD/2D 游戏对象 |
| 世界空间 Sprite | 3D Transform，受 Camera/深度影响 | `Scene::Render` → 普通 `RenderCommand` | 另作 `WorldSpriteComponent` 或模式，不要把屏幕 Sprite 的像素尺寸误当世界单位；billboard 后续再加 |
| 交互 UI | `UIContext` 的布局矩形、层级与输入焦点 | `UIContext::Submit` → screen view/quad | 独立树；初版由 Application 持有，不能仅靠 Scene 的组件渲染回调完成命中和捕获 |

可以共享 Texture、UV/tint 和最终 quad 绘制逻辑，但不强求这三者同属一个 Transform 或同一个资产生命周期。`Scene::Render2D` 当前对整个对象树遍历一遍是可接受的起点；将来内容多时可为 2D renderer 建注册列表/可见性筛选。

### SpriteComponent 的最小数据和接口

`Scene/Components/SpriteComponent.h/.cpp`：持有 `shared_ptr<Texture>`、UV rectangle、RGBA tint、renderOrder，可选 screen effect。尺寸初期沿用 `GameObject::SetScale2D`，原点默认 quad 中心（与 `MeshFactory::CreatePlane(1,1)` 一致），先不要再引入相互冲突的 width/height 字段。`OnRender2D(RenderQueue&)` 只构造命令，不直接 Bind 或访问 OpenGL；`IsValid()` 检查 owner/texture。资产来源由 `AssetManager::LoadTexture` 或之后的 `SpriteAssetLoader` 负责，组件构造不依赖 `Engine::GetInstance()`（当前 `SpriteTest` 仅作为测试）。可提供 `SetTexture`、`SetUVRect`、`SetTint`、`SetRenderOrder`、`SetEffect`，校验 UV 和无穷值。纹理像素、UV 原点、`ImageLoader` 的垂直翻转要约定并做一张非对称测试图；alpha 合成初版采用 straight alpha 与 `SrcAlpha/OneMinusSrcAlpha`，不要混入预乘 alpha 资源而不调整 BlendState。

如果需要“像素大小自动取自图片”，在组件层显式加 `SizeMode::{Explicit, NativePixels}`；不要通过 Texture 尺寸偷偷覆盖对象缩放。透明图片默认 screen blend，但仍明确 alpha/sRGB 的约定，不以是否有 alpha 通道推断所有图片都半透明。

### UI 树、渲染与输入

`UIContext` 由 Application 持有，内部 `unique_ptr<UIElement>` 树；`UIElement` 保存父子关系、可见/启用状态、局部矩形、锚点与 renderOrder，布局计算世界屏幕矩形。`UIImage`/`ImageElement` 提交 quad；`UIText`/`TextElement` 提交字形 quad；`UIButton`/`ButtonElement` 组合背景和标签。初版固定布局+少量锚点即可，先不做 Flex/Grid、样式表、完整控件系统。

UI 不应通过 `Scene::Render2D` 再走一次场景对象树：`Application::Render` 提交世界 Scene、可选 Scene 2D，然后 `UIContext::Submit` 到自己的 screen view。现有 RenderQueue **总是**先执行全部 3D view、后执行全部 screen view；第一版可明定屏幕 UI 恒在最上层，但如果多个 viewport/窗口需交错绘制，改成按提交顺序保存混合 view 列表。每个 view 的 `renderOrder` 稳定排序，同层保持提交序；UI 命中测试按**反向绘制顺序**取最上层控件，避免“看见 A 却点到 B”。

`UIContext::ProcessInput` 在游戏控制器之前消费本帧有序事件。把 GLFW 鼠标逻辑窗口坐标映射到 screen view 的逻辑区域：先扣除 viewport 左下角在窗口中的对应偏移、处理顶部原点，再按 framebuffer/逻辑尺寸比例换算；视口外不命中。pointer down 捕获目标，up 时在同一可点击目标上才触发 `OnClick`；移出、隐藏、销毁、切场景时取消捕获。回调期间增删控件需延迟到事件分发安全点。返回 `UICapture` 让游戏相机/角色不再同时处理被 UI 消费的鼠标；当前控制器直接读全局 `InputManager`，需要应用层先加门控，随后再传入过滤后的输入视图。点击回调先用控件本地 `std::function` 或 Action ID，仍无需全局 EventBus。

### 文字和资源接口

在 Image/Button 稳定后做 `FontAssetLoader`、`FontFace`、`GlyphAtlas`、`TextLayout`、`TextElement`。字体文件/字形栅格化留在 Assets，UI 只处理排版与字形 quad；UTF-8 解码、advance/基线/换行、缺字 fallback 不可省略。字形覆盖率用 `R8` atlas 与专用 screen effect；若动态加载新字形，`RenderDevice` 需有后端无关的纹理局部更新接口，否则先预烘焙明确字库。中文示例应选覆盖所用字符的字体，不把 UTF-8 字节当字符。初版可逐字提交，出现性能压力后按纹理 atlas、裁剪区与叠放顺序合批。

## 建议实施顺序与验收

1. 修上述 P0 1–4：两类 view 结束帧都清理；screen 状态和 shader/纹理缓存与 3D 一致；Init/Shutdown 对称。验收至少连续两帧 `3D → UI → 3D`、第二帧不提交 screen、初始化 shader 故意失败。
2. 修 P1 5–7：两套窗口尺寸、viewport、零尺寸、同帧输入队列和滚轮累计。验收 resize/高 DPI/子 viewport 点击坐标、同帧 press+release。
3. 明确 screen shader 固定 uniform 合约，校验 view/command，并修 2D 负角旋转；将 `SpriteTest` 升为无 Engine 单例依赖的 `SpriteComponent`。验收不同 UV、半透明叠放、父子 Transform 和负旋转。
4. 建最小 `UIContext/UIElement/ImageElement` 与反向绘制顺序命中、输入捕获；初版按钮可先用无文字图片状态验证点击。验收控件重叠、回调删自身、游戏控制器不误响应。
5. 加字形 atlas 与 `TextElement`，再给 Button 组合文字。第二种真实 screen 效果出现时评估 `ScreenMaterial`；不要在此之前泛化。最后再做 UI 资产/Codec、裁剪与合批。

文档里的类名是职责建议，不是必须一次引入的接口承诺；每阶段都要保留现有 3D Scene 的回归测试。尤其不要因已经能画一张 screen quad 就跳过跨帧缓存、GPU 资源销毁和 framebuffer 尺寸问题。
