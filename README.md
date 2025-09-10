# SAT Solver and Sudoku GUI

一个基于DPLL算法的SAT求解器，包含数独求解功能和Windows图形界面。

## 功能

- **集成主程序**: 统一的入口点，支持SAT和数独两种模式
- **SAT求解器**: 实现DPLL算法，支持DIMACS CNF格式
- **数独求解**: 将数独问题转换为SAT问题求解
- **图形界面**: Windows GUI，支持交互式数独编辑和求解
- **性能优化**: 优化的解析器和内存管理

## 编译

```bash
# 编译所有组件
make all

# 仅编译SAT求解器
make sat_solver

# 仅编译GUI (Windows)
make display

# 仅编译集成主程序
make main

# 清理
make clean
```

## 使用

### 集成主程序 (推荐)
```bash
# 交互式模式选择
./main

# 直接运行SAT模式
./main SAT

# 直接运行数独模式
./main %-Sudoku

# 显示帮助
./main --help
```

**主程序特性**:
- **统一入口**: 一个可执行文件包含所有功能
- **交互式选择**: 启动时可以选择运行模式
- **命令行支持**: 支持直接指定模式运行
- **完整功能**: 包含SAT求解和数独GUI的所有功能

### 独立SAT求解器
```bash
# 基本用法
./sat_solver input.cnf

# 带选项
./sat_solver input.cnf --print --model --timeout 5000 --check
```

### 独立数独GUI
```bash
# 运行图形界面
./display.exe <sudoku_file>.txt
```

## 文件格式

### CNF格式 (DIMACS)
```
c 注释行
p cnf <变量数> <子句数>
<子句1> 0
<子句2> 0
```

### 结果格式 (.res)
```
s <状态>     # 1=可满足, 0=不可满足, -1=超时/错误
v <模型>     # 变量赋值 (如果可满足)
t <时间>     # 求解时间(毫秒)
```

## 内存池优化
项目实现了两种CNF解析器，通过内存池技术显著提升性能：

**标准解析器 (parser.c)**:
- 每个子句独立分配内存
- 多次malloc/free调用
- 内存碎片化严重

**优化解析器 (parser_opt.c)**:
- **内存池技术**: 所有字面量存储在连续内存池中
- **减少分配次数**: 动态扩展池大小，减少内存分配开销
- **缓存友好**: 连续内存访问提高缓存命中率
- **结构优化**: 子句只存储索引和长度，减少内存占用

### 性能比较
求解器会自动比较两种解析器的性能：

```bash
# 输出示例
parse_ms=15.2 parse_opt_ms=8.7 optimize=42.76%
```