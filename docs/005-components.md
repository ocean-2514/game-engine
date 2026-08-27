# 光照组件与渲染数据

## 当前实现

光照以场景组件存在，公共基类 `LightComponent` 保存颜色和强度，具体类型负责各自的数据：

- `DirectionalLightComponent`：方向、颜色、强度；方向来自所属 `GameObject` 的世界前向量。
- `PointLight`：世界位置、颜色、强度、有效范围。
- `SpotLight`：世界位置和方向、颜色、强度、有效范围以及内外锥角。

灯光组件只描述场景语义，不直接依赖 OpenGL，也不负责向 Shader 上传 uniform。颜色和强度会被限制为非负值，点光源和聚光灯的范围具有一个很小的正下限；聚光灯角度限制在 `[0, 89.9]` 度，并保持 `inner <= outer`，避免无效数据进入渲染层。

## 数据流

每个 `RenderView` 都保存自己的 `CameraData` 和 `LightingData`。`Scene::Render` 在提交该视图的物体前递归收集存活对象上的灯光组件，并把世界空间的灯光快照写入 `LightingData`。之后 `RenderQueue` 在某个 Shader 第一次用于该视图时，上传一次相机和灯光数据。

因此当前约定是：

- `LightingData` 是某个视图执行期间使用的值快照，不持有组件指针。
- 位置、方向、片元位置以及相机位置全部使用世界空间。
- 同一对象可以具有不同类别的灯光组件，收集时各类型相互独立。
- 已进入延迟销毁状态的对象和组件不参与收集。

这一结构允许以后让不同视图使用不同的灯光筛选结果，而不需要给每条 `RenderCommand` 重复附带灯光数据。

## Shader 约定

默认 Shader 当前使用 Blinn-Phong 光照。材质的 `uDiffuse`、`uSpecular`、漫反射贴图和可选高光贴图共同决定表面颜色；环境光只影响漫反射颜色。透明度来自漫反射纹理、`uOpacity` 和可选 opacity 纹理，Masked 材质再通过 `uAlphaCutoff` 丢弃片元。

点光和聚光的 `range` 是有效范围：距离超过范围后贡献为零，范围边缘平滑衰减。Shader 对零距离、零范围以及重合锥角也有保护，避免除零产生 `NaN`。模型导入器通过 `uHasSpecularMap` 明确区分“没有高光贴图”，防止未绑定的 sampler 意外采样漫反射贴图所在的纹理单元。

当前 OpenGL/GLSL 3.30 路径仍使用固定大小的 uniform 数组：方向光 2 个、点光 16 个、聚光 8 个。C++ 的 `LightingLimits` 必须与 GLSL 中的 `MAX_*_LIGHTS` 保持一致，超出的灯光会被忽略。

## 后续可演进项

- 把灯光上限作为 Shader 构建配置生成，避免 C++ 与 GLSL 手动维护两份常量。
- 灯光数量稳定增大后，将逐项字符串 uniform 上传替换为 UBO；需要更多或动态数量时再考虑 SSBO、纹理缓冲或 clustered/forward+ lighting。
- 给 `RenderView` 增加光照层掩码、距离/视锥筛选，使每个视图只收集可能产生贡献的灯光。
- 将阴影视图和阴影贴图作为独立渲染 pass；不要把阴影资源塞入单个物体的 `RenderCommand`。
- 在引入 HDR 后明确灯光强度的物理单位、色彩空间以及 tone mapping。目前的强度与衰减属于直观的非物理模型。
