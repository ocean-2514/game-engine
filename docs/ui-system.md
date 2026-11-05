# UI 系统

UI 是应用层拥有的屏幕空间控件树，不属于 Scene 的 GameObject 树。应用创建 `UIContext`，通过它挂载、布局、路由指针输入并向 `RenderQueue` 提交绘制命令。UI 不会自动接管 Engine 生命周期或 gameplay 输入。

## 控件与所有权

- `UIElement` 是基类，保存父子关系、矩形、可见性、启用状态、命中测试设置和 RenderOrder。
- `UIImage` 持有 `shared_ptr<Texture>`，可设置 UV 区域和 tint。
- `UIButton` 派生自 UIImage，可复制另一个 UIImage 的外观作为背景、设置文本 label，并注册点击监听器。
- `UIText` 绘制一个文本框，不参与命中测试。文本可读出排版结果和 measured size。

`AddRoot` 和 `AttachElement` 接收 `unique_ptr`，调用后 UIContext 拥有节点。返回的 `UIElementHandle` 只在创建它的 context 中有效；删除后 generation 会变化，旧 handle 不会重新指向后来复用该槽位的节点。遍历或回调期间的添加、删除会排队到安全边界执行；此时新 handle 在命令提交前不能通过 `Resolve` 取得节点。删除父节点会递归删除整棵子树。

## 坐标与布局

`SetRect` 设置父节点坐标系中的逻辑像素矩形 `{x, y, width, height}`。根节点相对于 UI view 原点，子节点相对于父节点左上角。当前布局是绝对布局：不会自动 stretch、dock、堆叠、裁剪或根据内容调整父节点尺寸。

每帧应以实际窗口和 framebuffer 尺寸更新 ScreenView。`logicalWidth/Height` 使用窗口逻辑尺寸，`viewport` 使用 framebuffer 像素，`origin` 选择 `TopLeft` 或 `BottomLeft`。输入坐标由 UIContext 从窗口逻辑坐标映射到 viewport，再转换为 UI view 的逻辑坐标。文本的物理像素比例取 viewport 高度与逻辑高度之比；保持 X/Y 比例一致可避免字形变形。

`Visible` 节点正常绘制和交互；`Hidden` 节点及其子树不绘制、不交互；`Collapsed` 还将自身布局尺寸归零。禁用节点仍绘制但不参与命中；`HitTestVisible(false)` 只跳过该节点，仍允许命中子节点。默认命中测试不裁剪到父节点矩形。

## 图片、按钮与提交

```cpp
eng::FontAssetDesc fontDesc;
fontDesc.path = "font/example.ttf";
auto font = assets.LoadFont(fontDesc);

eng::TextureAssetDesc textureDesc;
textureDesc.path = "texture/button.png";
auto texture = assets.LoadTexture(textureDesc);
auto button = std::make_unique<eng::UIButton>();
button->SetRect({24.0f, 24.0f, 180.0f, 48.0f});
button->SetTexture(texture);
button->SetLabel(font, "Start");
button->AddClickListener([] { /* start the game */ });
uiContext->AddRoot(std::move(button));
```

`UIButton::SetBackground(const UIImage*)` 只复制来源图像当时的 Texture、UV 和 tint，不保留来源控件指针。按钮只响应左键；同一次按下/释放手势必须从按钮内开始，并在按钮矩形和 UI view 内结束才会触发监听器。指针离开后重新进入再释放仍可触发。隐藏、禁用、移除或显式取消会结束 capture，不会产生点击。

在游戏循环中先设置 view，再处理本帧输入；渲染时在其他屏幕内容之后提交 UI：

```cpp
uiContext->SetView({
    static_cast<float>(window->GetWindowWidth()),
    static_cast<float>(window->GetWindowHeight()),
    eng::ScreenOrigin::TopLeft,
    {0, 0, window->GetFrameBufferWidth(), window->GetFrameBufferHeight()}
});

const eng::WindowMetrics metrics{
    static_cast<float>(window->GetWindowWidth()),
    static_cast<float>(window->GetWindowHeight()),
    window->GetFrameBufferWidth(),
    window->GetFrameBufferHeight()
};
const eng::UICapture capture = uiContext->ProcessInput(input.GetEvents(), metrics);

// Render scene and other screen-space content first.
uiContext->Submit(queue);
```

`ProcessInput` 返回的 `UICapture` 当前只提供 `pointer` 标志；`keyboard` 始终为 false，因为键盘焦点和导航尚未实现。应用需自行决定 capture 是否屏蔽 gameplay 输入。当前示例调用 ProcessInput，但第三人称控制器仍直接读取 InputManager，因此两者可能同时响应。窗口失焦或 UI 停用时调用 `CancelPointerInput()`。

UIContext 按稳定的 RenderOrder 绘制节点，命中测试使用反向顺序，因此最后绘制的重叠节点优先命中。需让 UI 覆盖 Scene 的 screen-space 绘制时，必须在这些内容之后调用 `Submit`。

## 字体与文本

通过 `AssetManager::LoadFont` 加载字体。`FontAssetDesc::path` 是相对于 `assets/` 的路径；返回的字体可传给 UIText 或 UIButton label。当前示例在 `Game.cpp` 中从 `font/仿宋_GB2312.ttf` 加载字体。

```cpp
eng::FontAssetDesc fontDesc;
fontDesc.path = "font/example.ttf";
auto font = assets.LoadFont(fontDesc);
auto text = std::make_unique<eng::UIText>(font);
text->SetRect({24.0f, 88.0f, 360.0f, 120.0f});
text->SetText("Hello, 世界");
eng::TextStyle style;
style.pixelSize = 24.0f;
style.color = {1.0f, 1.0f, 1.0f, 1.0f};
style.align = eng::TextAlign::Left;
style.verticalAlign = eng::TextVerticalAlign::Top;
style.wrap = eng::TextWrap::Word;
style.lineSpacing = 1.0f;
style.pixelSnap = true;
style.tabSize = 4;
text->SetStyle(style);
uiContext->AddRoot(std::move(text));
```

`TextStyle::pixelSize` 是逻辑像素字号；FreeType 按 UI view 的物理像素比例栅格化字形。水平对齐支持 Left/Center/Right，垂直对齐支持 Top/Middle/Bottom，换行支持 None/Word/Character。布局缓存只在文本、字体、样式中的排版参数、矩形尺寸或像素比例变化时重建；单纯移动和颜色变化不触发重新排版。可在 `UIContext::Layout()` 后读取 `GetMeasuredSize()` 和 `GetLayout()`。

文本实现会解码 UTF-8，应用字形度量和 kerning，并使用字形图集生成 ScreenQuadCommand。Word 模式中的 CJK 断行是小型启发式规则，不是完整 Unicode line-break 算法；当前也没有复杂文字 shaping、双向文字、富文本或文本输入编辑控件。缺失字形会使用字体 fallback/.notdef 路径，可通过布局结果中的 `missingGlyphCount` 检查。

## 当前边界与测试

- 没有自动布局、父子裁剪、滚动容器、键盘焦点/导航、无障碍语义或完整控件主题。
- 没有通用 UI 文档/样式格式或资源驱动 UI 加载器；应用代码负责构造控件树。
- UICapture 尚未由 Engine 自动路由到 gameplay。
- 无窗口逻辑回归测试覆盖 UI 树、句柄、命中/capture、按钮、文本布局和 UIText 集成。启用 `ENGINE_BUILD_TESTS` 后可运行这些 CTest；测试不替代实际 framebuffer/字体资源的可视验收。

更多模块边界见 [架构导览](architecture-guide.md)，示例集成见 `src/Game/Game.cpp`。