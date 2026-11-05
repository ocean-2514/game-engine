# 项目概览

## 项目目标

这是一个用于学习和深入研究的 C++ 游戏引擎项目。目标不是一次构造完整的“通用引擎”，而是在保持可运行示例的前提下，逐步理解并实现游戏循环、资源管理、渲染提交、场景对象、组件生命周期、固定步长物理和动画等真实引擎问题。

当前渲染实现以 OpenGL 为中心。部分公共接口刻意不暴露 OpenGL 类型，以便隔离 API 细节；这是一种渐进式架构边界，不代表已经支持 Vulkan 或其他图形后端。

## 功能概况

| 子系统 | 当前实现 | 主要边界 / 未完成事项 |
| --- | --- | --- |
| Engine / Platform | GLFW 窗口、主循环、输入事件快照、窗口与 framebuffer 尺寸 | 当前构建与依赖配置是 Windows/MinGW 导向；平台失焦等事件覆盖有限 |
| Rendering | OpenGL 后端、RenderDevice、RenderQueue、材质、网格、纹理、3D/屏幕 quad、RenderState | 只有 OpenGL 实现；没有 RenderGraph、现代 GPU 资源/管线模型或 Vulkan 后端 |
| Scene | Scene、GameObject、层级、Component 生命周期与延迟变更、JSON 加载 | 没有完整场景编辑器；运行时 API 与序列化类型仍在扩展 |
| Assets | FileSystem、AssetManager、JSON Material、Texture、Assimp Model 与 Scene 加载 | 没有通用导入器 UI、打包器、热重载或完整异步加载流程 |
| Physics | Scene 拥有 PhysicsWorld，Bullet 后端，固定步长、刚体、碰撞形状、Raycast、事件和插值 | 没有形状共享缓存、刚体约束、角色控制器和调试绘制；事件仍以查询值为主 |
| Animation | 骨骼动画、动画切换、状态机、参数、Trigger、1D Blend Tree | 更复杂的动画层、混合树、事件、根运动与资产化控制器仍可继续完善 |
| Audio | Engine 级 AudioManager、AudioComponent、AudioListenerComponent，基于 miniaudio | 没有混音器 UI、空间遮挡、音频事件图和复杂资源流式策略 |
| 2D / UI | 世界/屏幕 2D Sprite；UIContext 树、绝对布局、图片、按钮、字体文本、指针命中与 capture | 没有自动布局/裁剪/滚动、键盘焦点/导航、无障碍或完整控件主题 |

上表描述代码现状的高层快照，不是完整测试保证。开始实现新功能前，应以头文件、实现和当前示例为准；设计讨论文档可能记录了尚未实现或已过期的方案。

## 主要工程问题与实现方式

### 渲染后端边界

游戏与场景侧通过 RenderDevice、RenderCommand、RenderState、Mesh、Texture 等引擎类型表达绘制意图；OpenGL 名称与状态映射集中在 `Renderer/OpenGL/`。这让引擎前端不必在每个组件里直接调用 GL，但后端仍按 OpenGL 的工作模式设计，尚未验证 Vulkan 所需的显式资源、同步和 pipeline 生命周期。

### 生命周期与安全变更

Scene 独占 GameObject 树，GameObject 独占组件及子对象；资源对象可由 `shared_ptr` 在 Model、Material、Texture、RenderCommand 等边界共享。Scene 更新和 UI 遍历的结构变化使用各自的有序命令队列延迟提交，避免回调过程中直接改动正在遍历的容器。句柄使用 generation 避免槽位复用后旧引用误指向新对象。

### 可诊断的资产实例化

Scene JSON 不由各组件直接读取。Parser 生成描述数据并验证 schema；DependencyResolver 加载模型、材质等依赖；Instantiator 创建对象、组件并连接引用。错误携带资产路径和 JSON 路径，便于定位到具体字段。AssetManager 聚合 texture/material/model/scene loader，并向需要共享贴图的加载器注入依赖。

### 多种更新时钟

游戏逻辑使用帧 delta；物理使用 Scene 内部固定步长累计器，限制每帧最大子步和积压时间；渲染时可在上一物理状态和当前状态之间做临时插值。动画目前跟随组件帧更新。这样能把物理稳定性与显示帧率分开，但完整的暂停、时间缩放、慢动作和统一 FixedUpdate 调度仍可继续设计。

## 项目技术栈

| 层次 | 技术 / 代码 | 用途 |
| --- | --- | --- |
| 语言与构建 | C++17、CMake | 静态引擎库与示例可执行文件 |
| 窗口与图形 | GLFW、GLAD、OpenGL 3.3 Core | 上下文、平台输入回调、OpenGL 函数入口 |
| 数学 | GLM | Transform、矩阵、向量、四元数与插值 |
| 模型导入 | Assimp | 导入常见模型/动画文件到引擎 Model/Mesh 数据 |
| 物理 | Bullet 2 | 刚体、形状、碰撞检测和求解 |
| 音频 | miniaudio | 设备、音频声源与播放 |
| 字体 | FreeType | 字形度量、栅格化与 UI 字形图集 |
| 图像 / JSON | stb_image、RapidJSON | 图片解码、材质与场景描述解析 |

GLFW/Assimp 当前使用随仓库提供的 Windows MinGW 链接文件；其余依赖的许可证应随第三方源码/头文件检查。项目没有统一的依赖管理器锁文件或跨平台 CI。

## 目录结构

```text
.
├── assets/                  示例场景、模型、Shader、纹理、材质和音频
├── docs/                    面向仓库读者的概览、构建和架构文档
├── engine/
│   ├── include/             随项目使用的第三方头文件/源码
│   ├── thirdparty/          Bullet 与 FreeType 源码
│   └── src/
│       ├── Assets/          资源描述、加载、场景 Codec/Registry
│       ├── Audio/           音频管理
│       ├── Input/           平台无关输入状态与事件
│       ├── Physics/         物理前端与 Bullet 实现
│       ├── Platform/        Window 抽象与 GLFW 实现
│       ├── Renderer/        渲染接口、命令队列与 OpenGL 后端
│       ├── Scene/           Scene、GameObject 与 Components
│       └── UI/              UI 树、图片/按钮、文本布局与绘制
├── src/                     示例应用与游戏逻辑
├── include/                 示例应用头文件
├── lib/、bin/               当前 Windows/MinGW 第三方链接库与运行 DLL
└── CMakeLists.txt           示例程序、引擎子目录与可选逻辑测试
```

## 示例程序

入口是 `src/main.cpp`，示例应用位于 `src/Game/`。当前示例从 `assets/scenes/scene1.json` 加载场景，再由游戏代码创建角色、相机与控制器。UIContext 中有一个带图片背景和文本 label 的按钮，以及独立的 UTF-8 文本元素；字体通过 AssetManager 从 `assets/font/` 加载。这个组合主要用于验证系统集成，不是功能完整的演示关卡。

运行时可以用 `W/A/S/D` 移动、`Space` 跳跃、鼠标移动调整第三人称视角、滚轮调节视野；`Q/E` 为竖直移动，移动时按鼠标右键切换行走/奔跑。示例 UI 按钮位于屏幕左上角，点击会向控制台输出文字。上述行为属于示例 Game/Script，并不意味着引擎提供统一的 Action Mapping；UICapture API 尚未接入角色输入门控。

场景文件包含版本、名称、对象 ID、Transform 和类型化组件定义。组件的 JSON 格式由 SceneTypeRegistry 与各 ComponentCodec 决定；不能仅根据 C++ 构造函数推断 JSON 字段。可以从 `assets/scenes/scene1.json` 和 `engine/src/Assets/Scene/ComponentCodec/` 开始阅读。


UI 的控件、坐标、文本、输入和渲染提交方式见 [UI 系统](ui-system.md)。

## 文档

- [构建与运行](build-and-run.md)
- [架构导览](architecture-guide.md)
- [UI 系统](ui-system.md)