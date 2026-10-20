# 声音系统架构（miniaudio）

- 状态：设计方案，尚未实现引擎音频类
- 目标依赖：仓库中的 miniaudio 0.11.25（`engine/include/miniaudio/`）
- 最后更新：2026-10-19

## 目标与首版范围

首版支持音频设备初始化、2D/3D 播放、音量/循环/暂停、随 GameObject 移动的声源、一个活动监听者，以及可复用的短音效和流式背景音乐。声音系统不应依赖 OpenGL 或 RenderQueue；音频播放由 miniaudio 的设备线程推进，游戏主线程负责提交控制和空间参数。

先不做录音、混响、遮挡、物理声学、音频事件图、编辑器混音台和多监听者混音。保留后续引入 bus、资源打包和异步加载的空间。公开头文件只使用引擎的 `Audio*` 类型、GLM 和标准库，不暴露 `ma_engine`、`ma_sound` 等 miniaudio 结构。

## 职责与所有权

```text
Engine
├─ AssetManager / FileSystem
├─ AudioManager（唯一设备、混音图、声音实例）
└─ Application
   └─ Scene（活动监听者及场景声源注册）
      └─ GameObject
         ├─ AudioComponent（一个持续存在的声源实例）
         └─ AudioListenerComponent（监听者候选）
```

`AudioManager` 由 Engine 持有并控制初始化/销毁。它可以采用 PImpl，在 `.cpp` 内持有 `ma_engine` 和每个 `ma_sound`；不要同时引入一套泛化音频后端接口。`ma_engine` 已包含设备、节点图和默认资源管理器，首版可直接使用。场景不拥有设备；多个 Scene 可以依次使用同一个 AudioManager，Scene 销毁或切换时必须先撤销它注册的声源和监听者。

`AudioComponent` 是**一个播放实例**，不是音频文件本身。同一文件同时播放多次需要多个实例。它持有引擎定义的 `AudioVoiceHandle`（索引 + generation，或等效安全句柄），不直接持有 `ma_sound*`。`AudioListenerComponent` 标记一个 GameObject 可以作为听音位置，也不直接持有 miniaudio listener index。Scene 只负责选择当前活动监听者，实际 index 0 由 AudioManager 管理。这样不会把监听设备的全局状态放到组件里。

当前 Scene 并非 Engine 所有，因此需要显式注入 AudioManager：例如 `Scene::SetAudioManager(AudioManager*)`，只能在首批对象 attach 前调用；或让 Scene 构造函数接收服务。前一种方式与现有默认构造、`SceneInstantiator` 较容易衔接。`AudioManager` 必须比所有已绑定 Scene 存活更久。不要在 `AudioComponent::OnAttach` 中调用 `Engine::GetInstance()`。

## 建议文件

```text
engine/src/Audio/
├─ AudioTypes.h                 AudioSourceDesc、AudioPlaybackState、AudioVoiceHandle 等
├─ AudioManager.h/.cpp          设备、声音实例、主音量、更新及销毁；miniaudio 只在 .cpp
└─ （以后）AudioClip.h/.cpp      需要显式预加载/资源句柄时再加

engine/src/Scene/Components/
├─ AudioComponent.h/.cpp        声源配置、播放控制、OnAttach/OnDetach
└─ AudioListenerComponent.h/.cpp 监听者候选组件

engine/src/Assets/Scene/ComponentCodec/
├─ AudioComponentCodec.cpp      场景 JSON 解析、依赖校验、组件构造
└─ AudioListenerComponentCodec.cpp
```

首版不必创建 `AudioWorld`、`AudioSource` 基类、`IAudioBackend` 或独立 `AudioAssetLoader`。将来需要热切换设备、离线混音或复杂资产管线时，再依据实际职责拆分。`AudioManager` 名称沿用项目的 `AssetManager`/`InputManager` 风格，但它实际是音频运行服务。

仓库已将 `engine/include/miniaudio/miniaudio.c` 列入 `ENGINE_SRC`。实现时核实它仅编译一次，并将该目录设为 engine 的 **PRIVATE** include 路径；新增的 `.cpp/.h` 也要列入 `engine/CMakeLists.txt`。`miniaudio.c` 已定义 `MINIAUDIO_IMPLEMENTATION`，其他翻译单元只包含声明，不重复定义宏。

## 公共接口草案

接口仅示意语义，具体命名可随实现调整：

```cpp
enum class AudioLoadMode { Decode, Stream };
enum class AudioPlaybackState { Stopped, Playing, Paused };

struct AudioSourceDesc {
    std::string assetPath;                 // 相对 assets 根目录
    AudioLoadMode loadMode = AudioLoadMode::Decode;
    bool spatial = true;
    bool loop = false;
    bool playOnAttach = false;
    float volume = 1.0f;                   // 线性增益，非 dB
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
};

struct AudioVoiceHandle {
    uint32_t index = 0;
    uint32_t generation = 0;
};

class AudioManager {
public:
    bool Init();
    void Shutdown();
    bool IsReady() const;
    void SetMasterVolume(float linearGain);

    // 供 AudioComponent 使用；失败返回结果/错误，不产生有效 handle。
    AudioResult<AudioVoiceHandle> CreateVoice(const AudioSourceDesc& desc);
    void DestroyVoice(AudioVoiceHandle handle);
    bool Play(AudioVoiceHandle handle);
    bool Pause(AudioVoiceHandle handle);
    bool Stop(AudioVoiceHandle handle);
    bool SetTransform(AudioVoiceHandle handle,
        const glm::vec3& position, const glm::vec3& velocity);
    bool SetListener(const AudioListenerState& state); // 使用 listener index 0
    void ClearListener();
    AudioResult<AudioVoiceHandle> PlayOneShot(
        const std::string& assetPath,
        const std::optional<glm::vec3>& worldPosition = std::nullopt);
    void Update(); // 回收已结束的一次性播放、处理主线程事件
};

class AudioComponent final : public Component {
public:
    ENG_COMPONENT_TYPE(AudioComponent);
    explicit AudioComponent(AudioSourceDesc desc);
    bool Play();
    bool Pause();
    bool Stop();
    void SetVolume(float linearGain);
    AudioPlaybackState GetPlaybackState() const;
protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;
};

class AudioListenerComponent final : public Component {
public:
    ENG_COMPONENT_TYPE(AudioListenerComponent);
protected:
    void OnAttach(Scene& scene) override;
    void OnDetach(Scene& scene) override;
};
```

`AudioResult<T>` 可以是项目现有 `SceneLoadResult` 的音频专用轻量对应物，也可以先用 `bool + out handle + 错误码`；不要为了这个接口新建通用异常框架。句柄的 generation 防止组件删除后旧句柄误操作复用的声音槽。若首版保证组件与 voice 一一对应，也可由 AudioManager 内部维护稳定地址对象，但不能向组件泄露易失效的裸 `ma_sound*`。

组件配置需要明确校验：`assetPath` 非空且位于 assets 根目录；`volume >= 0`、`pitch > 0`、`0 <= minDistance <= maxDistance`，所有浮点数有限。`Pause` 要保留当前播放位置；`Stop` 的语义约定为停止并将下次 `Play` 从头开始。miniaudio 的 `ma_sound_stop()` 本身不等同于清除播放游标，实现 `Stop` 时需显式 seek 到开头。播放到结尾后的再次 `Play` 也应按该规则重播。

## 初始化、更新和销毁时序

1. `Engine::Init` 建好 FileSystem 后初始化 AudioManager，并在 `Application::Init` 前准备好服务。音频设备失败应返回可诊断错误；首版可选择让 Engine 初始化失败。若以后允许无音频运行，需要显式的 silent/null 模式，不能把空设备伪装成成功。
2. `AssetManager::LoadScene` 创建 Scene 时注入 AudioManager，再提交 detached 对象。`AudioComponent::OnAttach` 创建 voice、设置参数和初始世界位置，然后按 `playOnAttach` 播放；Listener 的 `OnAttach` 只注册候选，不抢占活动监听者。
3. 每帧 `Scene::Update` 完成物理同步、普通更新和延迟操作冲刷后，统一同步活动监听者与已注册的活动声源世界变换，再调用 `AudioManager::Update` 处理结束的短暂 voice。不要在每个视图的 `Scene::Render` 中更新音频，否则多视图会重复提交，且当前渲染函数会临时修改刚体的插值变换。
4. `AudioComponent::OnDetach` 先销毁 voice，再清掉句柄；`AudioListenerComponent::OnDetach` 撤销候选。Scene `Clear`、延迟销毁、切换场景和 `Application::Destroy` 都应经过这些路径。`Engine::Destroy` 在销毁 Application/Scene 后销毁 AudioManager。

当前 `Component::OnAttach` 返回 `void`，因此设备/文件失败无法向 `SceneInstantiator` 报告并回滚。为了保持场景加载的失败原子性，音频依赖应在 `SceneDependencyResolver` 阶段验证并准备；运行期 attach 仍要报告/记录失败，组件保持未播放的无效 voice 状态。若以后要求所有 attach 失败都使场景加载失败，需要统一扩展组件 attach 契约，不能只对音频组件假设已经实现事务回滚。

## 监听者与空间同步

第一版每个 Scene 最多有一个**活动** AudioListenerComponent；可有多个候选用于切换。建议 `Scene::SetAudioListener(AudioListenerComponent*)` 和 `GetAudioListener()` 仿照主相机接口，并验证组件仍属于此 Scene 且存活。未指定活动监听者时，3D 声源静音或采用明确的固定默认监听位置；推荐静音并给诊断，避免声音凭空来自原点。2D UI/音乐不依赖监听者。

相机和监听者可以挂在同一 GameObject，也允许分开，例如旁观相机或过场镜头。`SetMainCamera` 不自动切换监听者。若希望初始场景默认“相机即耳朵”，由游戏或场景实例化阶段明确指定一个 AudioListenerComponent。场景 JSON 可选加 `settings.mainAudioListener`（对象 ID），与 `mainCamera` 一样在 link 阶段校验，且指向带一个 AudioListenerComponent 的对象。

坐标沿用引擎的世界空间：+Y 向上，GameObject 的 `GetWorldForward()` 为 −Z。将 listener 的 position、forward、up，以及 3D 声源的 position 传给 miniaudio。不要使用 Camera view 矩阵代替世界变换。首版 velocity 可固定为零，禁用/忽略 Doppler；实现速度时用相邻固定/帧快照除以有效 `deltaTime`，处理 teleport、暂停、零 `deltaTime` 和渲染插值，避免瞬时巨大 Doppler。场景 scale 不影响声音位置、音量或衰减距离。

对于 `spatial = false` 的音乐/UI，关闭声音空间化，忽略 GameObject 的位置；对 3D 声源设置最小/最大距离和选定衰减模型。把“世界单位对应多少米”在项目中统一规定（建议先按 1 单位 = 1 米），便于以后设定 Doppler、速度和物理声学。

## 资源、实例与一次性声音

场景和组件只保存 `assets` 相对路径。创建 voice 前用 `FileSystem::NormalizeAssetPath()` 校验和规范化，再转换为系统文件路径交给 `ma_sound_init_from_file()`；后者使用 miniaudio 的文件读取，不自动遵守项目的资源根目录。首版可以使用本地磁盘绝对路径。将来资源在包文件或虚拟文件系统内，再配置 miniaudio resource manager 的 VFS，避免绕过统一资产系统。

短音效使用解码入内存，长音乐使用 stream。仓库当前 miniaudio 构建默认内置解码 WAV、MP3、FLAC；Ogg/Vorbis 需要额外解码后端，不能仅凭文件扩展名假定可播放。miniaudio 的默认 resource manager 对同路径的非流式文件已经做引用计数/复用；流式实例有各自的流状态。首版先复用这一能力，避免在 AssetManager 里再做一套 `AudioClip` 缓存。`AssetManager` 后续可提供 `LoadAudioClip()`，届时要清楚区分“可共享的解码数据”和“每次播放独立的 voice”。

`AudioComponent` 适合环境声、机械声、背景音乐等与对象生命周期绑定的声音。枪声/碰撞等短暂声音建议由 `AudioManager::PlayOneShot(path, position?, options)` 创建独立 voice，并在主线程 `Update` 检测结束后回收；不要因为同一组件已经在播放就强行重启它。音频结束回调如果启用，只能投递轻量消息，不能在音频线程直接 `ma_sound_uninit()`、修改 Scene 或销毁 GameObject。

音量层级第一版只需 master + voice；背景音乐/音效分别调节的需求出现时，再加 `Music`/`SFX` 等 `ma_sound_group` 对应的引擎 `AudioBus`。不要把 bus 名称写死成底层 node 指针。切换设备、异步加载和资源热替换都留在 AudioManager/资产边界。

## 场景文件和 Codec

延续现有 `SceneTypeRegistry` 的 parse/resolve/create 三段式：

```json
{
  "type": "engine.AudioComponent",
  "asset": "audio/ambience/wind.mp3",
  "spatial": true,
  "loop": true,
  "playOnAttach": true,
  "volume": 0.6,
  "minDistance": 2.0,
  "maxDistance": 30.0,
  "loadMode": "stream"
}
```

`engine.AudioListenerComponent` 初版可只含 `type`。Parser 输出强类型 `AudioSourceDesc`；Resolver 验证路径和资源可用性；Factory 构造组件，AudioManager 由 Scene 在 attach 时提供。若要让场景加载的依赖验证覆盖解码失败，resolver 需要实际预加载/探测格式，而仅检查文件存在不足以保证可播放。初版明确保证路径和文件检查，解码与设备错误按运行期错误处理。

## 推荐实施顺序与验收

1. 完成 `AudioManager` 的 init/shutdown、2D voice、播放状态和稳定句柄；用最小示例验证同文件多实例、循环、暂停、停止重播、无效路径。
2. 接入 Engine 生命周期和 FileSystem 路径解析，检查无设备/空文件/重复 Shutdown 的行为。验证从 `build/bin` 启动仍可加载 assets。
3. 增加 `AudioComponent`、`AudioListenerComponent`，Scene 注册与每帧统一空间同步。验证移动、旋转、切换活动监听者、销毁组件/Scene 后没有残留声音。
4. 增加场景 Codec、`settings.mainAudioListener` 和资源错误路径。验证场景加载失败不会替换已有场景。
5. 增加一次性音效和必要的 Music/SFX bus；到这里再评估显式 AudioClip、异步加载和混音控制。

特别留意：`ma_engine` 和 `ma_sound` 都是需要稳定地址、显式 uninit 的结构；同一音频文件可有多个 voice；音频线程回调不做 Scene 操作；场景/组件先于 AudioManager 销毁；场景加载 attach 阶段尚无通用失败回滚能力。这几项是第一版最容易出现的生命周期问题。

## 初版实现记录（2026-10）

- `AudioManager` 由 Engine 初始化并在 Application/Scene 销毁后关闭。场景加载器必须在 `CommitObjects` 前注入它；否则组件挂载时无法创建 voice。
- `AudioComponent` 持有声源配置与稳定句柄；挂载时创建 voice、推送初始世界位置，再按 `playOnAttach` 播放。每帧在 `Scene::Update` 的命令冲刷之后同步位置。解绑时撤销注册并销毁 voice。`SetVolume` 同时更新实时音量和配置，允许挂载前调用。
- `AudioListenerComponent` 只是候选监听者。只有经 `Scene::SetAudioListener` 选中的组件会在每帧同步到设备；解绑活动监听者、清场或销毁 Scene 时会清除设备监听者。相机和监听者可以挂在同一对象，但不自动绑定。
- Codec 注册 `engine.AudioComponent` 与 `engine.AudioListenerComponent`。声源使用 `asset`、`spatial`、`loop`、`playOnAttach`、`volume`、`pitch`、`minDistance`、`maxDistance`、`loadMode`（`decode`/`stream`）；旧示例字段 `audio` 仍可解析。解析检查参数范围，依赖阶段检查资产路径及文件存在。监听者组件目前不需要额外字段。
- 可选 `settings.mainAudioListener` 引用带有唯一 `AudioListenerComponent` 的对象 ID，并在场景实例化后激活；也可由游戏代码调用 `SetAudioListener`。示例游戏将监听者挂在移动相机上，`scene1.json` 的风声配置为循环自动播放。
- 当前解析/依赖阶段不试解码音频。文件损坏、格式不支持、设备故障仍可能在 `OnAttach` 创建 voice 时失败；此时输出诊断并保持无效句柄，不加入场景同步列表。若以后需要场景加载的严格原子性，应增加音频预加载/探测和通用组件挂载失败传播机制。

## miniaudio 依据

- [miniaudio 官方手册：Engine、声音实例、资源管理、空间化与线程约束](https://miniaud.io/docs/manual/index.html)
- [miniaudio 官方空间音频示例：−Z 前向与监听者设置](https://miniaud.io/docs/examples/simple_spatialization.html)

接口和版本细节以仓库当前的 `engine/include/miniaudio/miniaudio.h` 为准。
