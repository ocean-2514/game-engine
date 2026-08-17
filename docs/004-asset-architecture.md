# 文件与资产架构

- 状态：当前实现约定与后续路线
- 最后更新：2026-08-18

## 模块职责

当前资产加载链路按职责拆分：

```text
FileSystem
├─ 定位 assets 根目录
├─ 校验并规范化资产相对路径
└─ 读取原始字节/文本
        │
        ├─ ImageLoader
        │  └─ 压缩图片字节 -> ImageData(TextureDesc + CPU pixels)
        │
        └─ MaterialAssetLoader
           ├─ JSON -> MaterialDesc
           ├─ 加载/缓存 ShaderProgram
           ├─ 加载/缓存 Texture
           └─ 构造/缓存运行时 Material
```

边界约定：

- FileSystem 不理解 JSON、Material、Image 或 GPU；
- ImageLoader 使用 stb_image 解码，但不调用 OpenGL；
- MaterialAssetLoader 负责资产格式、依赖解析和缓存，不包含后端 API；
- RenderDevice 只根据 Shader 源码、TextureDesc、SamplerDesc 和像素数据创建后端资源；
- OpenGLTexture 不读取文件，也不知道资源路径和图片编码格式。

## 资产路径

JSON 中的路径统一相对于 assets 根目录，并使用 `/`：

```json
"vertex": "shader/vs.glsl"
"value": "texture/container.jpg"
```

`FileSystem::NormalizeAssetPath()` 完成以下工作：

- 拒绝空路径和绝对路径；
- 对路径做 canonical/relative 规范化；
- 拒绝通过 `..` 或符号链接逃出 assets 根目录；
- 确认目标是普通文件；
- 返回使用 `/` 的规范化相对路径。

所有缓存均使用规范化路径，避免 `texture/a.jpg` 与 `texture/../texture/a.jpg` 形成重复条目。

## Material 加载

MaterialAssetLoader 先完整解析并验证 JSON，再创建资源。当前要求：

- 根节点为 object；
- `version` 是无符号整数 1；
- vertex/fragment Shader 路径存在，geometry 可选；
- parameters 是数组；
- 每个参数具有唯一且非空的 name；
- type 与 value 严格匹配；
- 当前支持 int、float、vec2、vec3 和 texture2D。

加载遵循失败原子性：

```text
解析完整 MaterialDesc
        ↓
创建/取得 Shader
        ↓
创建/取得全部 Texture
        ↓ 全部成功
构造并设置 Material
```

Shader 或任意 Texture 失败时返回 nullptr，不缓存失败，也不返回半初始化 Material。临时创建的资源由 shared_ptr 自动回收。

## 缓存所有权

MaterialAssetLoader 当前维护三个缓存：

```text
material path -> weak_ptr<Material>
ShaderAssetDesc -> weak_ptr<ShaderProgram>
TextureCacheKey -> weak_ptr<Texture>
```

缓存使用 weak_ptr，含义是“复用仍被外部使用的资源”，而不是“永久保活所有加载过的资源”。当 Scene、Material 等实际所有者释放最后一个 shared_ptr 后，GPU 资源可以正常销毁；下一次加载会重新创建。

每次 Material 加载前会清理三个缓存中的过期条目。`ClearCache()` 可以立即清空所有查找记录，但不会销毁仍由外部 shared_ptr 持有的资源。

MaterialAssetLoader 必须是长生命周期对象。如果每次加载前临时构造一个 Loader，实例之间无法共享缓存。当前示例由 Game 持有一个 Loader，并把它注入需要加载 Material 的对象。未来建立 AssetManager 后，Loader 和缓存可以由 AssetManager 统一持有。

## Material 缓存键与共享可变性

Material 缓存键是规范化后的 Material JSON 路径。同一路径在已有实例存活时返回同一个 shared_ptr<Material>。

当前 Material 是可变对象，因此：

```cpp
auto a = loader.Load("material/container.json");
auto b = loader.Load("material/container.json");
// a == b；通过 a 修改参数也会影响 b
```

这是当前“共享材质资产”的明确语义。如果以后同一基础材质需要每对象覆盖参数，应增加 MaterialInstance，或显式 Clone Material，而不是悄悄改变缓存行为。

## Shader 缓存键

Shader 缓存键包含规范化后的：

- vertex path；
- fragment path；
- geometry path。

只有三个阶段路径都相同才共享 ShaderProgram。Shader 编译失败不进入缓存。

当前键没有包含预处理宏、入口点或编译选项，因为这些功能尚未实现。未来加入 Shader variant 后，它们必须成为缓存键的一部分。

## Texture 缓存键

Texture 不能只用路径或解码后的 TextureDesc 作为缓存键。

仅使用 TextureDesc 会有两个问题：

1. 必须先读取和解码图片才能得到宽高，失去避免重复解码的意义；
2. 不同源文件可能恰好具有相同尺寸和格式，却不是同一纹理。

当前 TextureCacheKey 在解码前构造，并包含：

```text
规范化资产路径
├─ srgb
├─ flipVertically
├─ generateMipmaps
└─ SamplerDesc
   ├─ addressU / addressV
   ├─ minFilter / magFilter
   └─ mipFilter
```

这些字段都会影响 CPU 像素结果、GPU 内部格式、mip 数据或当前 OpenGLTexture 的采样状态，因此任何字段不同都不能错误共享同一个 Texture。

缓存查询发生在读取和解码之前。命中有效 weak_ptr 时不会再次读取图片、调用 stb_image 或创建 GPU Texture。解码/创建失败不写入缓存。

## SamplerDesc 的当前语义

虽然已经把 TextureDesc 和 SamplerDesc 分开描述，OpenGL 后端目前仍通过 `glTexParameteri` 把 SamplerDesc 固化到 Texture Object 上。因此 SamplerDesc 必须包含在 TextureCacheKey 中。

未来实现独立 Sampler 资源后，可以演进为：

```text
Image/Texture cache key
└─ path + color space + flip + mip generation

Sampler cache key
└─ SamplerDesc

Material binding
└─ Texture + Sampler
```

届时同一份 GPU 图像可以配合多个 Sampler 使用，SamplerDesc 应从 Texture 缓存键中移除。

## 当前限制与未来设计

### 文件变更与热重载

缓存键目前不包含文件修改时间或内容哈希。如果磁盘文件在运行时改变，只要旧资源仍存活，Loader 仍会返回缓存对象。

未来热重载可以采用：

- FileSystem watcher 产生资产变更事件；
- 维护资产依赖图：Material -> Shader/Texture；
- 对受影响 key 主动失效；
- 原地更新资源或创建新版本并安全切换；
- Material revision 变化后让 RenderQueue 重新应用参数。

在实现依赖图以前，`ClearCache()` 只能清除查找记录；已经返回给外部的 Material/Texture 不会自动重载或失效。

### 线程安全

MaterialAssetLoader、三个 unordered_map 缓存和 OpenGL RenderDevice 当前都按单线程使用设计。不要从多个工作线程同时调用 Load。

未来异步加载应拆成：

```text
工作线程：文件读取、JSON 解析、图片解码
                         ↓ CPU 结果队列
渲染线程：Shader 编译、Texture GPU 上传、Material 最终组装
```

届时缓存需要互斥、任务去重或 loading future，避免多个线程同时加载同一个 key。

### AssetManager

MaterialAssetLoader 当前同时承担特定资产解析和小型缓存职责，适合现阶段。资产类型增加后可引入 AssetManager：

- 统一拥有各类型 Loader；
- 统一规范化路径和缓存策略；
- 缓存 Shader、Texture、Material、Mesh 等资源；
- 管理异步任务、依赖、失败信息和热重载；
- 提供默认/错误资源。

在出现第二、第三种文件资产和跨资产依赖以前，不需要立即构建通用句柄、反射注册或复杂 Asset Database。
