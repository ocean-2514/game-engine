# OpenGL Learning Engine

一个以 C++17 和 OpenGL 为基础、用于学习与逐步实践游戏引擎架构的个人项目。它不是成熟的通用游戏引擎；当前重点是把渲染、场景、资产、物理、动画、音频和 UI 等子系统以可运行的小步迭代连接起来。

仓库入口程序会创建窗口、初始化引擎并运行示例游戏。示例加载 JSON 场景，创建带物理与动画的角色、摄像机和基础 UI。可从这里了解项目：

- [项目概览](docs/project-overview.md)：功能范围、技术重点、简历描述参考与成熟度说明。
- [构建与运行](docs/build-and-run.md)：依赖、CMake 命令与常见问题。
- [架构导览](docs/architecture-guide.md)：模块职责、运行流程和扩展方向。

## 当前已有

- OpenGL 渲染后端、RenderDevice/RenderQueue、3D 与屏幕空间 quad 渲染。
- Scene、GameObject、Component、摄像机、灯光、Sprite 和初版 UI 图片/按钮。
- JSON 场景加载、材质/纹理/模型资产加载，模型导入使用 Assimp。
- Bullet 刚体、碰撞形状、固定步长更新、Raycast 和碰撞/Trigger 事件。
- 骨骼动画、Animator 状态机与 1D Blend Tree。
- 基于 miniaudio 的音频管理、声源和监听者组件。

“已有”表示源码中存在相应的初版实现，不表示功能已达到生产级稳定性；详情和限制见[项目概览](docs/project-overview.md)。

## 工程技术点

- 用引擎层描述类型隔离 OpenGL 对象和状态；目前只有 OpenGL 后端，明确保留未来扩展空间而不声称已跨 API。
- 用值类型 RenderCommand、分视图 RenderQueue 和显式 RenderState 组织渲染提交；按材质修订和 Shader/视图数据做有限缓存。
- 将场景加载拆为 JSON 描述解析、依赖解析、对象/组件实例化，使用注册表与 Codec 维护类型 schema 和字段错误定位。
- Scene/GameObject/Component 采用树形所有权与生命周期注册；遍历期间的结构修改在安全点执行。
- Scene 物理使用固定时间步；动画状态机可用类型化参数和条件 Transition 驱动，并支持 1D Blend Tree 与姿态交叉淡化。

这些点适合描述为“实现与验证中的引擎子系统”，不应表述为已达到商业引擎级的性能、兼容性或工具链完整度。各模块的具体取舍见[架构导览](docs/architecture-guide.md)。

## 快速开始

当前构建配置面向 Windows + MinGW，使用仓库中的 GLFW/Assimp import libraries 和 DLL。需要 CMake 3.50 或更新版本，以及可用的 MinGW-w64 GCC/G++ 工具链。

```powershell
cmake -S . -B build -DENGINE_BUILD_TESTS=ON
cmake --build build --config Debug -j 4
ctest --test-dir build -C Debug --output-on-failure
```

运行 `build/bin/my_engine.exe`。开发构建会优先读取 CMake 配置时确定的源码 `assets/` 路径；资产运行与打包边界详见[构建与运行](docs/build-and-run.md)。

## 项目状态

项目仍在积极演进，接口和资产格式可能变化。尚未提供完整编辑器、正式发布包、跨平台 CI、稳定性承诺或 Vulkan 后端；UI、资源管线和各子系统也仍有未实现能力。仓库根目录目前没有项目级许可证文件，复用代码或资源前请先确认其各自许可证及来源说明。

## 许可证与资源

仓库根目录当前没有项目级许可证文件。第三方依赖和示例模型/纹理可能适用各自不同的许可证或署名要求；在分发修改版之前，请逐项核对相应文件中的许可和来源信息。
