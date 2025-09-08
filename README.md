## SAT 求解器（DPLL）

基于 DPLL 算法（单子句传播 + 分裂/回溯）的简易 SAT 求解器，支持 DIMACS CNF 格式输入，输出与算例同名的 `.res` 结果文件，并提供超时控制与结果校验。

### 目录结构

- `parser.h/.c`：DIMACS CNF 解析与打印
- `solver.h/.c`：DPLL 求解、回溯、模型校验
- `main.c`：命令行参数解析、计时、结果输出
- `parser_opt.h/.c`：优化版解析器（池化字面量、连续内存），用于对比性能
- `Makefile`：构建与运行脚本（目标名为 `main`）
- `cases/`：示例算例

### 构建

```bash
make
```

### 使用


```bash
./main <input.cnf> [--print] [--model] [--timeout MS] [--check]
```

参数说明：

- `--print`：打印解析后的 CNF（用于人工核对解析正确性）
- `--model`：当结果为 SAT 时，在控制台打印一个满足赋值（DIMACS 风格，末尾 0）
- `--timeout MS`：设置求解超时时间（毫秒）。`<=0` 表示无超时
- `--check`：在计时外验证得到的模型是否满足 CNF，并打印 `check: OK/FAIL/ERROR`


示例：

```bash
./main cases/sat-20.cnf --timeout 5000 --model --check
```

批量运行：

```bash
for f in cases/*.cnf; do ./main "$f" --timeout 5000; done
```
### 输入格式（DIMACS CNF）

- 头：`p cnf <num_vars> <num_clauses>`
- 注释：以 `c` 开头的整行
- 子句：由整数组成，以 `0` 结束；正数为正文字，负数为负文字

### 解析器优化
- 原始解析器（`parser.c`）逐子句累加到临时数组并在遇到 0 后为该子句单独分配内存，再拷贝字面量。
- 优化解析器（`parser_opt.c`）使用单个整型连续池 `literals_pool` 存放所有字面量，每个子句仅记录 `start_index` 与 `num_literals`，显著减少小块分配与拷贝；池按需二倍扩容。
- 程序会分别计时：`parse_ms` 为原始解析耗时；`parse_opt_ms` 为“优化解析 + 转换为标准 CNF”的合计耗时。控制台会打印两者与优化率 `optimize=(parse_ms - parse_opt_ms)/parse_ms`。
- 求解阶段始终基于原始解析得到的 `CNF` 运行 DPLL，DPLL 计时 `t` 不包含任何解析/转换时间，且不受上述比较影响。
- 若输入实际子句数少于头部声明，两个解析器都会按实际解析到的子句数进行调整；注释行（以 `c` 开头）与空白会被忽略。

### 输出（.res 文件）

程序会在与输入同目录生成同名 `.res` 文件，包含三行：

- `s <结果>`：1=SAT，0=UNSAT，-1=超时
- `v <模型>`：仅在 SAT 时输出，每个变量对应的字面量值（如 `1 -2 ...`），空格分隔
- `t <毫秒>`：DPLL 执行时间（毫秒；不包含 `--print`/`--check`/模型打印时间）

示例（SAT）：

```
s 1
v 1 -2 -3 4 ...
t 17
```

示例（UNSAT）：

```
s 0
t 3
```

### 实现要点

- DPLL：单子句传播（反复应用）、分裂选择第一个未赋值变量
- 回溯：在每次分支前快照整个赋值数组，确保传播副作用不泄漏
- 计时：`clock()` 记录 DPLL 内部执行时间（毫秒）


