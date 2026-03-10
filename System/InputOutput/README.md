# InputOutput 数据接口说明

## 新增 SEG-Y 数据接口
- `segy_data_interface.h/.cpp` 提供 `SegyDataInterface`：
  - 读取 SEG-Y 二进制头（采样间隔、每道采样点数、数据格式）。
  - 读取全部道数据（当前支持 SEG-Y format=5，即 IEEE 浮点）。
  - 将读取结果直接写出为 SEP 文件对（`.H` + `.@`）。

## CMake 管理
从仓库根目录构建：

```bash
cmake -S . -B build
cmake --build build -j
```

会生成可执行程序：`build/System/InputOutput/segy_to_sep`

## 转换命令
```bash
build/System/InputOutput/segy_to_sep input.segy output.H output.@
```

## 推荐内部落盘格式
建议在整个地震处理流程中统一使用 **SEP 格式（头文件 `.H` + 数据文件 `.@`）** 作为内部格式。
原因：
1. 仓库内现有多数处理模块依赖 `SEPReader/SEPWriter`。
2. 处理链条衔接成本低，减少格式转换损耗。
3. 头信息与数据体分离，便于批处理脚本组织。
