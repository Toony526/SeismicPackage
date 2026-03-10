# InputOutput 数据接口说明

## SEG-Y 数据接口
- `segy_data_interface.h/.cpp` 提供 `SegyDataInterface`：
  - 读取 SEG-Y 二进制头（采样间隔、每道采样点数、数据格式）。
  - 支持 SEG-Y 样本格式 `1/2/3/5/8`。
  - 当 binary header 的采样点数/采样间隔为 0 时，会自动回退读取首道 trace header 字段。
  - 按块（chunk）读取+转换+写出到 SEP，避免全量一次性内存占用。
  - 支持块内多线程解码（`decode_threads`）。

### 当前支持的 SEG-Y 样本格式
- format 1: IBM 4-byte floating point
- format 2: 4-byte two's complement integer
- format 3: 2-byte two's complement integer
- format 5: 4-byte IEEE floating point
- format 8: 1-byte two's complement integer

## CMake 管理
从仓库根目录构建：

```bash
cmake -S . -B build
cmake --build build -j
```

会生成可执行程序：`bin/segy_to_sep`  
会生成验证程序：`bin/segy_sep_validate`  
静态库会生成在：`lib/libseismic_io.a`

## 转换命令
默认输出扩展名已改为：`.header` + `.trace`（避免 `@` 特殊符号带来的命令引用问题）。

```bash
# 默认输出：input.header + input.trace
bin/segy_to_sep input.segy

# 显式指定输出
bin/segy_to_sep input.segy output.header output.trace

# 指定块大小和解码线程
bin/segy_to_sep input.segy output.header output.trace 4096 8
```

## 转换正确性验证
推荐在转换后执行：

```bash
bin/segy_sep_validate input.segy output.header
```

该工具会检查：
1. 样点数（SEG-Y vs SEP `n1`）一致。
2. 道数（SEG-Y vs SEP 总道数）一致。
3. 样点逐点差异（`max_abs_diff`、`rmse`）。
4. 非有限值统计（`non_finite_samples`）；若该值大于 0，程序会返回非 0 状态。

## 可视化查看道头与道数据
- 道数据可视化：
  - `SEPDataViewer` 可查看转换后的 `output.header`。
  - `SEGYDataViewer` 可查看原始 SEG-Y。
- 道头查看：
  - `SEPReader` 可读取 `n1..n8`、`hdr_label`、`sort_order`。
  - SEG-Y 道头可在 `SegyDataInterface` 基础上扩展输出。

## 推荐内部落盘格式
建议整个流程统一使用 SEP 的 `.header + .trace`。
