# 板2 AT、UART 透传与输出控制

## 通信约定

- UART1、UART2、UART3 均为 115200、8N1、无硬件流控。
- AT 命令必须全大写并以 `\r\n` 结束，不允许空格或制表符。
- 合法命令成功执行后返回 `OK\r\n`。
- 非 AT 的 UART1 CRLF 帧原样发送到当前已启用的 UART2、UART3；没有启用目标时静默丢弃。
- UART2、UART3 收到的数据以大写十六进制事件返回 UART1，单个事件最多携带 32 字节：
  - `+UART2RX:<HEX>\r\n`
  - `+UART3RX:<HEX>\r\n`

## 命令

| 命令 | 说明 |
| --- | --- |
| `AT+NMOS1=ON` / `OFF` | 控制 PB4；ON 输出低电平 |
| `AT+NMOS2=ON` / `OFF` | 控制 PB15；ON 输出低电平 |
| `AT+NMOS3=ON` / `OFF` | 控制 PB6；ON 输出低电平 |
| `AT+PWM=<0..100>` | 设置 PB9/TIM4_CH4 的整数百分比占空比 |
| `AT+12V=ON` / `OFF` | 控制 PB12；低电平开启 12V Buck |
| `AT+18V=ON` / `OFF` | 控制 PB3；低电平开启 18V Buck |
| `AT+STATUS=?` | 查询 12V、18V、NMOS1/2/3 和 PWM 当前软件状态 |
| `AT+UART2=ON` / `OFF` | 启用或关闭 UART2 透传 |
| `AT+UART3=ON` / `OFF` | 启用或关闭 UART3 透传 |
| `AT+UART2&3=ON` / `OFF` | 同时启用或关闭 UART2、UART3 |
| `AT+UARTTX=<HEX>` | 向当前已启用目标发送 1～32 字节二进制数据 |

`AT+UARTTX` 只接受大写、偶数位十六进制字符。例如 `AT+UARTTX=00FF10\r\n` 发送三个字节 `00 FF 10`。

## 错误响应

| 响应 | 原因 |
| --- | --- |
| `+ERROR:PARSE\r\n` | 未知命令、格式错误、非法字符或参数 |
| `+ERROR:RANGE\r\n` | PWM 超过 100，或 HEX 数据超过 32 字节 |
| `+ERROR:UART_DISABLED\r\n` | 执行 `AT+UARTTX` 时没有启用目标 |
| `+ERROR:UART_TX\r\n` | HAL 串口发送失败或超时 |
| `+ERROR:LINE_TOO_LONG\r\n` | UART1 输入帧超过 127 字节 |
| `+ERROR:RX_OVERFLOW\r\n` | 软件环形缓冲溢出，受影响的缓存已清空 |
| `+ERROR:12V_DISABLED\r\n` | 12V 未开启，不能开启 NMOS |
| `+ERROR:18V_DISABLED\r\n` | 18V 未开启，不能设置非零 PWM |

`AT+STATUS=?` 返回示例：

```text
+STATUS:12V=OFF,18V=OFF,NMOS1=OFF,NMOS2=OFF,NMOS3=OFF,PWM=0
OK
```

关闭 12V 会自动关闭三路 NMOS；关闭 18V 会自动把 PWM 设为 0%。

## 自动验证

```powershell
.\tests\run_host_tests.ps1

$env:PATH='C:\Users\44575\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\13.3.1+st.9\bin;C:\Users\44575\AppData\Local\stm32cube\bundles\cmake\4.0.1+st.3\bin;C:\Users\44575\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin;'+$env:PATH
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

## 实板验收清单

1. 上电复位期间测量 PB4、PB15、PB6，确认三路均为高电平关闭状态。
2. 确认 FreeRTOS 启动后 PA8 保持高电平，LED2 常亮。
3. 在 PB9 测量约 1 kHz PWM；依次发送 0%、25%、50%、100%，确认占空比对应。
4. 分别发送三路 NMOS 的 ON/OFF 指令，确认 ON 为低电平、OFF 为高电平。
5. 单独启用 UART2、单独启用 UART3、同时启用两路，验证 UART1 非 AT 帧的目标选择。
6. 从 UART2/UART3 注入包含 `00`、`0D`、`0A`、`FF` 的数据，确认 UART1 返回对应 HEX 事件。
7. 让三路串口连续收发并触发一次线缆断连/重连，确认接收中断能够继续工作。
