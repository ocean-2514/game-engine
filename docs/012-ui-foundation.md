# UIContext 与 UIElement 初版

本轮实现的是 UI 基础设施，不包含 UIImage、UIButton、UIText、字体、锚点/自动布局或裁剪。详细后续路线见 `drafts/ui-system-implementation-plan.md`。

## 所有权与句柄

- UIContext 的槽位通过 unique_ptr 独占元素；父子关系只保存句柄，不重复拥有节点。
- 句柄只在创建它的 UIContext 内使用，不允许跨 Context 传递。Context 不可复制或移动。
- 槽位有 reserved 状态；待挂载元素已预留索引，但 `Resolve()` 返回 nullptr。挂载后才返回借用指针。删除后 generation 递增，旧句柄失效；generation 耗尽的槽位不再复用。
- 遍历期 AddRoot/AttachElement/Remove 是有序命令。可以在同一回调里添加父节点、为其添加子节点，再删除子节点；Resolve 仍需等待安全点。
- 每次布局/提交遍历结束、每个输入事件结束后执行命令。若先删除父节点再执行挂载子节点，挂载失败并释放子节点槽位。返回的 staged 句柄不保证最终能挂载。
- 删除节点递归删除子树，同时从父节点或根列表解除引用。保持兄弟顺序，不采用 swap-and-pop。
- 虚函数中可以申请树结构变更，但不能直接 delete 元素或销毁当前 UIContext；回调应正常返回，不依赖异常进行流程控制。

## 布局与状态

初版仅支持相对父节点的绝对矩形。根节点相对 view 原点，子节点位置为父 arrangedRect 的位置加 localRect 的位置；父尺寸不会自动缩放子节点。

`Layout()` 从根向下检查脏状态。父节点变化会重排后代；干净节点不重复调用 OnLayout。OnLayout 可以设置子节点的矩形；修改自身矩形、在布局回调中新增的节点在下一次 Layout 生效，不无限递归重排。同样，不应在 OnSubmit 中修改布局。

Hidden 保留布局尺寸，但整棵子树不绘制、不命中；Collapsed 自身 arranged 尺寸为零，整棵子树不绘制、不命中。目前没有自动流式布局，因此 Collapsed 不会自动挪动兄弟节点。Enabled=false 仍绘制，但整棵子树不交互。

`SetHitTestVisible(false)` 仅让自身不参与命中，不影响子节点，适合根容器或装饰背景。默认 true；无可见内容的根容器建议主动关闭自身命中，避免阻挡游戏输入。

OnVisibilityChanged 通知本节点的本地 visibility 变化，不是祖先变化后的有效可见性通知。状态 setter 的回调同样处于结构变更保护范围内；隐藏、禁用或关闭命中时，会取消已不合法的 hover/capture。

## 排序与绘制

先按根/子节点插入顺序生成前序遍历，再按元素的有符号 renderOrder 稳定升序排序。相同 order 下父节点先于子节点、后添加的兄弟更靠上；显式 order 可以跨父子/根节点排序，初版不建立独立 stacking context。

命中按同一列表逆序查找。当前没有父矩形裁剪：子节点可以绘制和命中于父矩形之外，但不能突破祖先的隐藏/禁用状态。

派生控件仅在 OnSubmit 中向 UIDrawList 追加自己的命令，不应删除/重排其他控件命令。UIContext 为追加的命令统一设置本次遍历的元素 order，再经 BeginScreenView/SubmitScreenQuad/EndScreenView 提交。自定义控件不需要直接接触 OpenGL 或操作共享 Material。

提交遍历中的结构变更在遍历结束后生效，当前已生成的绘制值仍属于本次快照；RenderQueue 不持有 UIElement 指针。

## 输入与坐标

ProcessInput 接收窗口逻辑坐标的有序 InputEvent 与 WindowMetrics。应用必须先 SetView，再处理输入。Context 将坐标经 framebuffer DPI 比例和 viewport 偏移映射到 view 逻辑坐标，同时支持 TopLeft/BottomLeft；元素局部坐标再减去 arrangedRect 的位置。

目前支持 Enter、Leave、Move、左键 Down/Up/Cancel 和 Scroll。首次 Down 命中最上方目标后捕获该目标，后续移出 viewport 仍向原目标发送 Move/Up。Up 不等于 Click：按钮应自行判断释放是否在自身内，再触发点击。

每个输入事件结束后刷新有序结构命令，所以下一个事件不会投递给上一个事件中已删除的元素。没有输入事件时也重新检查目标，处理布局或状态改变后的 hover。删除捕获节点会先发送 Cancel，删除 hover 节点发送 Leave。

返回的 UICapture.pointer 为“本帧任一时刻被 UI 使用或仍被 UI 占用”；即使同帧按下后移出并释放，也不会把这次手势泄漏给游戏。keyboard 当前始终 false，尚未实现键盘焦点、导航、文字输入或事件冒泡。mods 原样转发；当前平台输入层尚未填充真实修饰键快照，不应依赖它实现组合键。

窗口失焦、切换 UI 或停止给 UI 输入前，应用应调用 `CancelPointerInput()`。目前 Window 没有 focus 事件通路，本轮未擅自扩展平台层；平台失焦自动通知需要后续接入。窗口尺寸无效时 ProcessInput 自动取消目标。UI 不会清除 InputManager 的全局状态：游戏相机/控制器仍需尊重 UICapture。

## 应用调用顺序

```cpp
// Window::PollEvents 与 InputManager::BeginFrame 后，在 gameplay Update 前：
ui.SetView(view); // 使用最新逻辑尺寸和 framebuffer viewport；相同值不会重排。
auto capture = ui.ProcessInput(input.GetEvents(), metrics);
// 用 capture.pointer 控制游戏鼠标输入；不要让 UI 和镜头同时响应。

// Application::Render 中，Scene 的 screen view 提交结束后：
ui.Submit(queue); // 内部调用 Layout，不应嵌套在另一个 BeginScreenView 中。
```

## 验证

`engine/test/UIContextTests.cpp` 包含五组无窗口回归测试：布局/句柄、输入捕获、DPI/子 viewport 与状态传播、有序延迟操作、提交排序。构建和运行：

```text
cmake -S . -B build -DENGINE_BUILD_TESTS=ON
cmake --build build --config Debug -j 4
ctest --test-dir build -C Debug --output-on-failure
```

测试不初始化 OpenGL，验证的是逻辑而非实际图片输出；控件和图形效果仍需后续可视化验收。
