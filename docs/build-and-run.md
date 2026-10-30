# 构建与运行

## 支持范围

仓库的 CMake 配置要求 CMake 3.50 及以上、C/C++ 编译器和 C++17。当前仓库提供 `libglfw3dll.a`、`libassimp.dll.a` 等 MinGW import libraries，以及对应 Windows DLL，因此本说明仅覆盖 Windows + MinGW-w64。其他编译器或操作系统目前没有验证，不应视作已支持。

Bullet 2 以源码加入构建；GLFW 和 Assimp 使用仓库提供的预编译链接文件。GLM、GLAD、stb_image、RapidJSON 和 miniaudio 的头文件或源码随仓库提供。若调整编译器/架构，预编译库必须与目标 ABI 匹配。

## 构建

在仓库根目录打开 PowerShell 或 MinGW 可用的终端：

```powershell
cmake -S . -B build -DENGINE_BUILD_TESTS=ON
cmake --build build --config Debug -j 4
```

不需要逻辑回归测试时，可将 `ENGINE_BUILD_TESTS` 设为 `OFF` 或省略。构建系统会在 `build/bin/` 生成 `my_engine.exe`，并将 `assets/` 复制到构建目录。根目录 CMake 配置还会尝试将 `bin/` 中的 GLFW/Assimp DLL 复制到程序输出目录；若 CMake 提示 DLL 缺失，请检查仓库依赖文件是否完整。

运行：

```powershell
.\build\bin\my_engine.exe
```

开发构建通过生成的配置头将 `assets/` 源目录提供给 FileSystem，因此本地运行通常不依赖当前工作目录。若将可执行文件移动到另一目录，FileSystem 的回退路径要求 `assets/` 位于可执行文件旁边；当前仓库尚未提供可搬运的安装/打包流程。示例会创建 OpenGL 窗口并加载场景、模型、Shader 和纹理；音频设备不可用时，Engine 初始化也可能失败。

## 测试

当前可选测试主要覆盖 UI 的无窗口逻辑，不代表渲染画面、音频设备或物理行为已有完整自动化验收：

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

测试目标由根 CMake 的 `ENGINE_BUILD_TESTS` 选项控制。未来添加新系统时，应尽量把无需窗口/设备的纯逻辑测试与人工运行的图形烟雾测试分开。

## 常见问题

### CMake 找不到编译器或依赖库

确认 CMake 能找到 MinGW-w64 的 GCC/G++，且仓库 `lib/` 下的 import libraries 与编译器 ABI 匹配。不要将 MSVC 编译器与 MinGW `.a` 文件混用。

### 程序启动时找不到 DLL

检查 `bin/glfw3.dll` 和 `bin/libassimp-6.dll` 是否存在，并确保构建后的 `build/bin/` 中有副本。也可确认 DLL 位数与 executable 一致。

### 资产路径报错

JSON/代码中的资源路径按 `assets/` 根目录的相对路径传入，例如 `scenes/scene1.json`、`texture/container.jpg`；不要传绝对路径。优先确认从仓库根目录启动，并检查构建目录下的 `assets/` 复制结果。

### 黑屏或 OpenGL 初始化失败

确认驱动支持项目请求的 OpenGL 3.3 Core Profile、GLFW 能创建窗口，并检查程序输出的 shader 编译/链接错误。当前文档不承诺特定显卡或远程桌面环境均可运行。
