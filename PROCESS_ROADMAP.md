# SeismicPackage 地震处理流程路线图

本文将仓库中的模块按典型地震数据处理流程进行排序，帮助快速确定“先看什么、后看什么”。

## 1. 数据读取与基础设施（入口层）
- `System/InputOutput`：SEP/SEGY 读写基础组件，几乎所有算法模块都依赖。
- `SEGYDataViewer` / `SEPDataViewer`：数据浏览与快速质检。
- `HeaderManipulation`：道头管理与排序字段处理。

## 2. 数据整理与预处理
- `TraceSort`：按炮/检波点/道集进行排序。
- `TraceMerge`：多批次道集合并。
- `Resample`：时间采样率重采样。
- `Pad`：补零与尺寸对齐。
- `SmoothingFilter2D`：二维平滑去噪。
- `AmplitudeBalancing`：振幅均衡。
- `GeometricSpreadingCorrection`：几何扩散补偿。
- `Mute1D` / `DesignMute`：静音窗设计与应用。

## 3. 速度相关处理与时差校正
- `NMO`：正/反 NMO 校正，基于速度函数插值。
- `CalculateGradient`：速度模型或目标函数相关梯度计算（用于后续迭代/优化）。
- `VelocityModelViewer` / `VelocityModelTiler`：速度模型检查与分块。

## 4. 变换域与波形属性处理
- `AutoCorrelation`：自相关分析。
- `HilbertTransform`：解析信号与瞬时属性基础。
- `FKTransform` / `FKTransform3D`：f-k 域变换与滤波。
- `FXTransform`：f-x 域变换。
- `Radon`：Radon 域去噪/分离。
- `PredictiveDeconvolution`：预测反褶积，提高分辨率。
- `ZeroPhasingFilter`：零相位滤波。
- `TransferFunctionViewer`：滤波/系统响应观察。

## 5. 建模、传播与旅行时
- `SyntheticSeismicGenerator` / `ModelingSeismicGenerator`：合成地震记录生成。
- `Modeling`：声波/地震正演建模。
- `WavePropagation` / `SphericalWavePropagation`：波场传播模拟。
- `RayTracing` / `GenerateTravelTimeCubes`：射线追踪与旅行时体生成。
- `PoyntingVector`：波场传播方向与成像条件辅助。

## 6. 偏移成像与高阶成像
- `KirchoffMigration`：Kirchhoff 偏移成像。
- `MapMigration`：基于旅行时映射的成像流程。
- `RTM`：逆时偏移（高精度成像核心模块）。

## 7. 插值、重建与多次波处理
- `RegularTraceInterpolation`：规则化道插值。
- `Kriging`：地统计插值重建。
- `SRME`：表面相关多次波消除。

## 8. 并行与分布式扩展（工程层）
- `ParallelModeling`：并行建模版本。
- `MapReduce`：分布式处理尝试。
- `System/MultiThreading`、`System/Networking`：并行与网络基础组件。

## 9. 理论与展示资料
- `AnisotropicRayTracingTheory` / `AnisotropicWavePropogationTheory`：各向异性理论推导与报告。
- `ExactAnisotropy`：各向异性精确计算实验。
- `AnisotropicPresentation` / `WavePropagationPictures`：展示图和结果图。

---

## 推荐学习顺序（最短路径）
1. `System/InputOutput` + `SEPDataViewer`
2. `TraceSort` → `Resample` → `SmoothingFilter2D` → `AmplitudeBalancing` → `Mute1D`
3. `NMO`
4. `FKTransform` / `Radon` / `PredictiveDeconvolution`
5. `RayTracing` + `GenerateTravelTimeCubes`
6. `KirchoffMigration`（先）→ `RTM`（后）
7. `SRME`、`RegularTraceInterpolation`、`Kriging` 作为增强模块

