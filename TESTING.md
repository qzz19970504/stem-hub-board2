# 板2固件测试流程

本文用于验证 `stem-hub-board2` 的主机纯逻辑、固件构建和实板行为。测试应按本文顺序执行：先完成不依赖硬件的检查，再烧录并进行上电安全、输出和串口测试。

## 测试范围与当前状态

| 范围 | 当前状态 |
| --- | --- |
| AT、行读取、环形缓冲、PWM 换算、HEX 编码主机测试 | 已自动验证 |
| Debug 固件构建 | 已自动验证 |
| Release 固件构建 | 已自动验证 |
| 实板烧录、电平、波形和三路 UART 联调 | 待连接板卡后执行 |

“已自动验证”只表示代码测试和交叉编译通过，不代表硬件电气行为已经测量。完成实板测试后，请填写文末验收记录。

## 测试设备与接线

### 所需设备

- 板2及稳定、限流合适的电源。
- ST-Link 或兼容 SWD 下载器。
- 至少一个 3.3 V TTL USB 转串口模块；完整双向联调建议准备三个。
- 示波器或逻辑分析仪，用于测量 PB9 PWM。
- 万用表或逻辑分析仪，用于测量 PA8、PB4、PB15、PB6。
- 支持选择 HEX/ASCII 发送及显式 CRLF 的串口工具。

### UART 接线

| MCU 串口 | MCU TX | MCU RX | 用途 |
| --- | --- | --- | --- |
| UART1 | PA9 | PA10 | AT 控制与事件输出 |
| UART2 | PA2 | PA3 | 外部设备 2 |
| UART3 | PB10 | PB11 | 外部设备 3 |

连接 USB 转串口时交叉连接 TX/RX：

- 转串口 RX 接 MCU TX。
- 转串口 TX 接 MCU RX。
- 转串口 GND 与板卡 GND 共地。
- 信号电平必须为 3.3 V TTL，不能直接连接 RS-232 电平。

三路串口参数均设置为：

```text
115200 baud
8 data bits
1 stop bit
No parity
No flow control
```

串口工具必须明确发送 `CRLF`。仅按 Enter 时，不同工具可能发送 LF，导致 AT 解析失败。

## 自动化测试

### 环境

主机测试脚本使用 PATH 中的 `gcc` 编译纯 C 模块：

```powershell
gcc --version
```

应显示支持 C11 的 GCC 版本。

### 执行

在仓库根目录运行：

```powershell
.\tests\run_host_tests.ps1
```

预期结果：

```text
PASS test_at_protocol
PASS test_line_reader
PASS test_ring_buffer
PASS test_output_math
PASS test_uart_tunnel
```

覆盖范围：

- 全部合法 AT 命令及严格大小写、CRLF、空白和未知命令。
- PWM 0%、1%、50%、100% 及越界、负数、非数字。
- HEX 空值、奇数位、小写、非法字符和 32 B 边界。
- CRLF 行读取、嵌入 NUL、超长行丢弃与恢复。
- 环形缓冲回绕、满缓冲、溢出恢复和顺序保持。
- UART2/3 HEX 事件编码。

## 固件构建验证

### 工具

确保以下命令可用：

```powershell
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

工程已使用 STM32Cube 附带的 CMake 4.0.1、Ninja 1.13.1、GNU Tools for STM32 13.3.1 验证。若普通 PowerShell 找不到这些命令，可使用 STM32CubeIDE/CubeCLT 配置好的终端，或把对应 bundle 的 `bin` 目录加入当前会话 PATH。

### Debug

```powershell
cmake --preset Debug
cmake --build --preset Debug --clean-first
```

检查：

- [ ] 编译及链接退出码为 0。
- [ ] App 模块无 `-Wall`、`-Wextra` 或 `-Werror` 错误。
- [ ] `build/Debug/stem-hub-board2.elf` 存在。
- [ ] RAM 小于 20 KiB。
- [ ] Flash 小于 64 KiB。

当前参考结果：

```text
RAM:   12648 B / 20 KiB
FLASH: 38872 B / 64 KiB
```

### Release

```powershell
cmake --preset Release
cmake --build --preset Release --clean-first
```

检查：

- [ ] 编译及链接退出码为 0。
- [ ] `build/Release/stem-hub-board2.elf` 存在。
- [ ] RAM 小于 20 KiB。
- [ ] Flash 小于 64 KiB。

当前参考结果：

```text
RAM:   12640 B / 20 KiB
FLASH: 22852 B / 64 KiB
```

编译器、链接器或 HAL 版本变化后，实际用量可以与参考值不同；容量不越界且没有新警告才是通过标准。

## 上电前检查

- [ ] 确认板卡型号和 MCU 为 STM32F103C8 对应设计。
- [ ] 确认供电电压、极性和限流设置正确。
- [ ] 确认三个 NMOS 负载不会在误导通时造成过流。
- [ ] 确认串口模块为 3.3 V TTL，并已共地。
- [ ] 检查 PA9/PA10、PA2/PA3、PB10/PB11 未接反。
- [ ] 使用 Debug 或 Release 的 `.elf` 烧录；若下载工具要求 HEX/BIN，先用 `arm-none-eabi-objcopy` 转换。
- [ ] 烧录完成后先保持 NMOS 负载处于可安全观察状态。

## 上电默认状态

前置条件：固件已烧录，示波器或万用表探头已接好，串口1已打开。

| 检查项 | 操作 | 预期结果 | 结果 |
| --- | --- | --- | --- |
| NMOS1 | 上电或复位，测量 PB4 | 高电平，NMOS1 关闭 | [ ] |
| NMOS2 | 上电或复位，测量 PB15 | 高电平，NMOS2 关闭 | [ ] |
| NMOS3 | 上电或复位，测量 PB6 | 高电平，NMOS3 关闭 | [ ] |
| PWM | 上电后测量 PB9 | 默认占空比 0% | [ ] |
| LED2 | 等待 FreeRTOS 启动，测量 PA8/观察 LED | PA8 高电平，LED2 常亮 | [ ] |
| 透传目标 | 直接发送 `AT+UARTTX=AA\r\n` | UART1 返回 `+ERROR:UART_DISABLED\r\n` | [ ] |
| 18V Buck | 上电后测量 PB3 | 高电平，18V 关闭 | [ ] |
| 12V Buck | 上电后测量 PB12 | 高电平，12V 关闭 | [ ] |

电源联锁快速验收：

1. `AT+STATUS=?` 应显示两路电源 OFF、三路 NMOS OFF、PWM=0。
2. 12V 关闭时发送 `AT+NMOS1=ON`，应返回 `+ERROR:12V_DISABLED`。
3. 开启 12V 后 NMOS1 可开启；关闭 12V 后状态中 NMOS1 自动变为 OFF。
4. 18V 关闭时发送 `AT+PWM=50`，应返回 `+ERROR:18V_DISABLED`。
5. 开启 18V 后可设置 PWM；关闭 18V 后状态中 PWM 自动变为 0。

如果任一 NMOS 上电后为低电平，应立即断开负载电源并检查 GPIO 初始化、烧录镜像和硬件上下拉。

## AT 协议测试

除特别说明外，所有输入都从 UART1 发送，末尾必须包含字节 `0D 0A`。

### 合法命令

| 输入（显示形式） | 预期 UART1 响应 | 结果 |
| --- | --- | --- |
| `AT+NMOS1=ON\r\n` | `OK\r\n` | [ ] |
| `AT+NMOS1=OFF\r\n` | `OK\r\n` | [ ] |
| `AT+NMOS2=ON\r\n` | `OK\r\n` | [ ] |
| `AT+NMOS2=OFF\r\n` | `OK\r\n` | [ ] |
| `AT+NMOS3=ON\r\n` | `OK\r\n` | [ ] |
| `AT+NMOS3=OFF\r\n` | `OK\r\n` | [ ] |
| `AT+PWM=0\r\n` | `OK\r\n` | [ ] |
| `AT+PWM=100\r\n` | `OK\r\n` | [ ] |
| `AT+UART2=ON\r\n` | `OK\r\n` | [ ] |
| `AT+UART2=OFF\r\n` | `OK\r\n` | [ ] |
| `AT+UART3=ON\r\n` | `OK\r\n` | [ ] |
| `AT+UART3=OFF\r\n` | `OK\r\n` | [ ] |
| `AT+UART2&3=ON\r\n` | `OK\r\n` | [ ] |
| `AT+UART2&3=OFF\r\n` | `OK\r\n` | [ ] |

### 格式和范围错误

| 输入或操作 | 预期 UART1 响应 | 结果 |
| --- | --- | --- |
| `at+PWM=50\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| 发送 `AT+PWM=50\n` | 暂无响应；尚未形成 CRLF 帧 | [ ] |
| `AT+PWM=50 \r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+UNKNOWN=ON\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+PWM=-1\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+PWM=A\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+PWM=101\r\n` | `+ERROR:RANGE\r\n` | [ ] |
| `AT+UARTTX=\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+UARTTX=0\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| `AT+UARTTX=00ff\r\n` | `+ERROR:PARSE\r\n` | [ ] |
| 发送超过 32 B 的大写偶数位 HEX | `+ERROR:RANGE\r\n` | [ ] |

LF-only 用例确认无响应后，再发送单独的 `\r\n` 结束当前异常帧；此时预期收到 `+ERROR:PARSE\r\n`，然后再执行后续用例。

完成该组后发送 `AT+NMOS1=OFF\r\n`、`AT+NMOS2=OFF\r\n`、`AT+NMOS3=OFF\r\n` 和 `AT+PWM=0\r\n`，恢复安全输出。

## NMOS 输出测试

测试时使用限流电源；如果连接实际负载，先确认低电平导通符合硬件设计。

| 步骤 | UART1 输入 | 预期响应 | 引脚预期 | 结果 |
| --- | --- | --- | --- | --- |
| 1 | `AT+NMOS1=ON\r\n` | `OK\r\n` | PB4 低电平 | [ ] |
| 2 | `AT+NMOS1=OFF\r\n` | `OK\r\n` | PB4 高电平 | [ ] |
| 3 | `AT+NMOS2=ON\r\n` | `OK\r\n` | PB15 低电平 | [ ] |
| 4 | `AT+NMOS2=OFF\r\n` | `OK\r\n` | PB15 高电平 | [ ] |
| 5 | `AT+NMOS3=ON\r\n` | `OK\r\n` | PB6 低电平 | [ ] |
| 6 | `AT+NMOS3=OFF\r\n` | `OK\r\n` | PB6 高电平 | [ ] |

- [ ] 测试结束后三路 NMOS 均已恢复 OFF。

## PWM 输出测试

在 PB9/TIM4_CH4 接示波器或逻辑分析仪。PWM 计数时钟为 1 MHz，ARR=999，因此频率约为 1 kHz。

| UART1 输入 | 预期响应 | 预期 PB9 波形 | 结果 |
| --- | --- | --- | --- |
| `AT+PWM=0\r\n` | `OK\r\n` | 约 1 kHz 配置下输出保持低，0% | [ ] |
| `AT+PWM=25\r\n` | `OK\r\n` | 约 1 kHz，25% | [ ] |
| `AT+PWM=50\r\n` | `OK\r\n` | 约 1 kHz，50% | [ ] |
| `AT+PWM=100\r\n` | `OK\r\n` | 输出保持高，100% | [ ] |
| `AT+PWM=101\r\n` | `+ERROR:RANGE\r\n` | 占空比保持上一合法值 | [ ] |

允许测量误差应根据仪器采样率和板级时钟误差确定。功能验收重点是频率接近 1 kHz，且四个合法占空比与命令一致。

- [ ] 测试结束后发送 `AT+PWM=0\r\n`。

## UART 透传测试

### UART1 到 UART2

1. 从 UART1 发送 `AT+UART2=ON\r\n`，预期 `OK\r\n`。
2. 保持 UART3 关闭。
3. 从 UART1 发送非 AT 帧 `HELLO2\r\n`。
4. UART2 应收到完全相同的字节：

```text
48 45 4C 4C 4F 32 0D 0A
```

- [ ] UART2 收到原始帧。
- [ ] UART3 没有收到该帧。

### UART1 到 UART3

1. 发送 `AT+UART2=OFF\r\n` 和 `AT+UART3=ON\r\n`，每条均应返回 `OK\r\n`。
2. 从 UART1 发送 `HELLO3\r\n`。
3. UART3 应收到：

```text
48 45 4C 4C 4F 33 0D 0A
```

- [ ] UART3 收到原始帧。
- [ ] UART2 没有收到该帧。

### UART1 同时到 UART2、UART3

1. 从 UART1 发送 `AT+UART2&3=ON\r\n`，预期 `OK\r\n`。
2. 发送 `BOTH\r\n`。
3. UART2、UART3 均应收到：

```text
42 4F 54 48 0D 0A
```

- [ ] UART2、UART3 均收到相同帧。

### `AT+UARTTX` 二进制发送

保持 UART2、UART3 均启用：

```text
AT+UARTTX=00FF100D0A\r\n
```

预期：

- UART1 返回 `OK\r\n`。
- UART2、UART3 均收到五个字节 `00 FF 10 0D 0A`。

- [ ] 双目标二进制发送正确。

然后发送 `AT+UART2&3=OFF\r\n`，再发送：

```text
AT+UARTTX=AA\r\n
```

预期 UART1 返回：

```text
+ERROR:UART_DISABLED\r\n
```

- [ ] 未启用目标时没有下行数据。

### UART2、UART3 到 UART1

1. 启用双目标：`AT+UART2&3=ON\r\n`。
2. 从 UART2 以 HEX 方式发送 `00 0D 0A FF`。
3. UART1 应收到：

```text
+UART2RX:000D0AFF\r\n
```

4. 从 UART3 以 HEX 方式发送 `12 34 AB CD`。
5. UART1 应收到：

```text
+UART3RX:1234ABCD\r\n
```

- [ ] UART2 上行事件的端口号、HEX 和 CRLF 正确。
- [ ] UART3 上行事件的端口号、HEX 和 CRLF 正确。

单次 HAL 接收事件和任务调度可能影响分块边界。连续输入超过 32 B 时允许产生多个 `+UARTxRX` 事件，但事件中的字节顺序必须与输入一致，每个事件最多携带 32 B。

### 关闭时清队列

1. 启用 UART2。
2. 从 UART2 连续注入数据，同时从 UART1 发送 `AT+UART2=OFF\r\n`。
3. 收到 OFF 的 `OK\r\n` 后停止注入。

通过标准：

- [ ] OFF 返回后不再出现关闭前遗留数据对应的 `+UART2RX` 事件。
- [ ] UART2 后续数据在再次启用前被静默清理。
- [ ] 再次发送 `AT+UART2=ON\r\n` 后，新接收数据可正常回传。

## 异常与恢复测试

### 超长 UART1 行

1. 从 UART1 连续发送至少 128 个非 CRLF 字节。
2. 预期 UART1 返回一次：

```text
+ERROR:LINE_TOO_LONG\r\n
```

3. 继续发送 `\r\n` 结束丢弃状态。
4. 发送 `AT+PWM=0\r\n`。

- [ ] UART1 返回 `OK\r\n`，证明下一帧可正常解析。

### RX 环形缓冲溢出

该测试需要制造任务来不及消费的情况，例如在调试器中暂停消费者任务后向对应 UART 高速注入超过 256 B，再恢复运行。普通 115200 短帧不一定稳定触发。

预期 UART1 返回：

```text
+ERROR:RX_OVERFLOW\r\n
```

通过标准：

- [ ] 受影响 UART 的队列被清空。
- [ ] 随后发送新命令或新数据可以正常处理。
- [ ] 没有重放溢出前的残留数据。

### UART 错误恢复

可通过调试器故障注入或制造 UART overrun/framing/noise error 验证。单纯断开 TX/RX 线通常不会让阻塞式发送返回失败。

通过标准：

- [ ] `HAL_UART_ErrorCallback()` 被调用后接收被 abort 并重新挂接。
- [ ] 清除故障条件后，该 UART 能继续触发 Receive-to-Idle 回调。
- [ ] 后续 AT 或桥接数据可正常处理。

### UART 发送失败

`+ERROR:UART_TX\r\n` 仅在已启用目标的 `HAL_UART_Transmit()` 返回非 `HAL_OK` 时产生。建议通过调试器或 HAL mock 强制一次下行发送失败。

- [ ] 故障注入时 UART1 返回 `+ERROR:UART_TX\r\n`。
- [ ] 解除故障后再次发送可返回 `OK\r\n`。

### 栈和 heap 故障钩子

此项是开发期可选故障注入测试：

- 人为缩小任务栈或制造栈溢出，确认进入 `vApplicationStackOverflowHook()`。
- 人为耗尽 FreeRTOS heap，确认进入 `vApplicationMallocFailedHook()`。

两者当前均调用 `Error_Handler()`。完成后必须还原配置，重新构建，并确认 git diff 不包含故障注入代码。

## 验收记录

| 项目 | 记录 |
| --- | --- |
| 测试日期 | |
| 测试人员 | |
| 板卡编号/版本 | |
| MCU 丝印 | |
| Git 提交 | |
| 固件配置 | Debug / Release |
| 电源电压与限流 | |
| 串口工具及版本 | |
| 示波器/逻辑分析仪 | |
| 主机测试 | 通过 / 失败 |
| Debug 构建 | 通过 / 失败 |
| Release 构建 | 通过 / 失败 |
| 上电默认状态 | 通过 / 失败 |
| NMOS 输出 | 通过 / 失败 |
| PWM 输出 | 通过 / 失败 |
| UART 透传 | 通过 / 失败 |
| 异常恢复 | 通过 / 失败 / 未执行 |
| 遗留问题 | |

最终结论：

- [ ] 通过，可进入下一阶段。
- [ ] 有条件通过，遗留问题已记录。
- [ ] 不通过，禁止连接正式负载。
