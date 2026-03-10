# InputOutput 数据接口说明

## 新增 SEG-Y 数据接口
- `segy_data_interface.h/.cpp` 提供 `SegyDataInterface`：
  - 读取 SEG-Y 二进制头（采样间隔、每道采样点数、数据格式）。
  - 读取全部道数据（支持 SEG-Y format=1/2/3/5/8）。
  - 将读取结果直接写出为 SEP 文件对（`.H` + `.@`）。

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
```bash
bin/segy_to_sep input.segy output.H output.@
```

## 转换正确性验证
推荐在转换后执行：

```bash
bin/segy_sep_validate input.segy output.H
```

该工具会检查：
1. 样点数（SEG-Y vs SEP `n1`）一致。
2. 道数（SEG-Y vs SEP 总道数）一致。
3. 样点逐点差异（`max_abs_diff`、`rmse`）。
4. 非有限值统计（`non_finite_samples`）；若该值大于 0，程序会返回非 0 状态，提示不能判定为严格一致。

## 可视化查看道头与道数据
- 道数据可视化：
  - `SEPDataViewer` 可直接查看转换后的 `output.H` 数据体（灰度显示随时间滚动）。
  - `SEGYDataViewer` 可用于原始 SEG-Y 的快速图形浏览。
- 道头查看：
  - `SEPReader` 可读取并打印 `n1..n8`、`hdr_label`、`sort_order` 等头信息（可在工具中按需输出）。
  - SEG-Y 道头可在 `SEGYReader`/`SegyDataInterface` 基础上扩展输出关键字段（炮号、检波点号、CDP 等）。

## 推荐内部落盘格式
建议在整个地震处理流程中统一使用 **SEP 格式（头文件 `.H` + 数据文件 `.@`）** 作为内部格式。
原因：
1. 仓库内现有多数处理模块依赖 `SEPReader/SEPWriter`。
2. 处理链条衔接成本低，减少格式转换损耗。
3. 头信息与数据体分离，便于批处理脚本组织。
