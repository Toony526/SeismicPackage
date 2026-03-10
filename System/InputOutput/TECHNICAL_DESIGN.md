# SEG-Y 批量转换为 SEP 的技术设计稿

## 1. 背景与目标
面对大量 SEG-Y 数据并发转换，核心挑战有三类：
1. 单机磁盘吞吐上限（读写争用）。
2. 当前串行读取/解码的 CPU 利用不足。
3. 集群场景下任务分发、容错与结果汇聚。

目标：
- 将转换流程从“全量读入内存后一次写出”升级为“分块流式转换”。
- 引入块内多线程解码，提升单机吞吐。
- 给出可落地的集群分布式调度方案。
- 输出扩展名统一为 `.header + .trace`，避免 `.@` 引用不便。

## 2. 现状问题
旧实现中 `write_as_sep` 依赖 `read_all_traces()`，会将整个 SEG-Y 数据一次性装入内存后写出；当文件大或并发多时，容易触发内存和 I/O 双瓶颈。

## 3. 新设计（已实现部分）

### 3.1 单机流式分块转换（已实现）
流程：
1. 读取 SEG-Y 二进制头，确定 `trace_count`、`samples_per_trace`、`sample_format`。
2. 按 `traces_per_chunk` 读取原始 trace block。
3. 在内存中将 block 解码为 float（支持 format 1/2/3/5/8）。
4. 将该 block 直接写入目标 SEP 对应偏移。
5. 重复直到所有 trace 完成。

收益：
- 峰值内存从 O(全文件) 降为 O(chunk)。
- 为并行化和分布式切分提供天然边界。

### 3.2 块内多线程解码（已实现）
- 新增 `decode_threads` 参数，按 trace 范围划分线程。
- 每线程负责不重叠 trace 区间，避免写冲突。
- `decode_threads=0` 时自动使用 `hardware_concurrency`。

### 3.3 输出扩展名调整（已实现）
- CLI 默认输出从 `.H/.@` 改为 `.header/.trace`。
- 也允许显式传入输出路径。

## 4. 集群分布式方案（设计，待实现）

### 4.1 任务切分
优先采用“按文件切分”，单大文件场景采用“按 trace range 切分”。
- 任务最小单元：`{file, trace_begin, trace_end}`。
- 每个 worker 执行本地流式分块转换。

### 4.2 调度与容错
- 使用中心任务表（可落地在 DB 或 KV）：`pending/running/success/failed`。
- 任务幂等：同一任务重复执行不会破坏结果（输出加 task-id 或原子 rename）。
- 节点故障回收：超时未 heartbeat 自动重派。

### 4.3 存储与网络建议
- 节点本地临时盘写分片，完毕后汇聚到共享存储。
- 控制并发 I/O 阈值，避免同时压垮共享 NAS。

## 5. 参数建议
- `traces_per_chunk`: 初始可用 2048 或 4096，根据磁盘和内存调优。
- `decode_threads`: 先设为物理核心数的一半到等量范围，观察 CPU 与 I/O 平衡。

## 6. 验证与观测
- 功能正确性：`segy_sep_validate input.segy output.header`。
- 性能观测：记录每文件转换耗时、MB/s、CPU 利用率、I/O 等待时间。

## 7. 后续迭代路线
1. 单机：读/解码/写三阶段 pipeline（带有界队列）。
2. 多文件并行批处理（worker pool）。
3. 集群调度服务 + 任务状态面板。
