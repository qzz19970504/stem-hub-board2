# Buck 电源控制、状态查询与负载联锁设计

## 目标

为 PB3/PB12 增加安全的 18V/12V Buck 使能控制，并在现有 AT 协议中增加电源控制、状态查询和负载联锁。完成主机测试、Debug/Release 构建、ST-Link 烧录及 485 实机命令验证后合并到 `master`。

## GPIO 与默认状态

- PB3 命名为 `BUCK18V_CTRL`，PB12 命名为 `BUCK12V_CTRL`。
- 两路均为反相控制：GPIO 高电平关闭 Buck，低电平开启 Buck。
- CubeMX 初值和 `App_OutputInit()` 均将两脚拉高，保证软件启动时默认关闭。
- PB3 原为 JTAG 引脚；工程已经执行 `__HAL_AFIO_REMAP_SWJ_NOJTAG()`，只保留 SWD，ST-Link 的 SWD 下载和调试不受影响。

## AT 接口

- `AT+18V=ON|OFF`
- `AT+12V=ON|OFF`
- `AT+STATUS=?`

设置成功返回 `OK\r\n`。状态查询返回两行：

```text
+STATUS:12V=OFF,18V=OFF,NMOS1=OFF,NMOS2=OFF,NMOS3=OFF,PWM=0\r\n
OK\r\n
```

状态反映 MCU 软件输出状态，不代表实际输出电压已经建立；当前硬件没有接入电压反馈测量。

## 安全联锁

- 12V 关闭时，`AT+NMOS1/2/3=ON` 不执行并返回 `+ERROR:12V_DISABLED\r\n`。
- 12V 关闭时，NMOS `OFF` 始终允许。
- 执行 `AT+12V=OFF` 时先关闭 NMOS1/2/3，再关闭 12V Buck。
- 18V 关闭时，非零 `AT+PWM=<n>` 不执行并返回 `+ERROR:18V_DISABLED\r\n`。
- 18V 关闭时，`AT+PWM=0` 始终允许。
- 执行 `AT+18V=OFF` 时先将 PWM 设为 0%，再关闭 18V Buck。

## 架构

`app_at_protocol` 只负责纯解析；`app_output` 是 Buck、NMOS 和 PWM 当前状态的唯一来源，封装安全联锁和 GPIO/TIM 写入；新增纯模块 `app_status` 负责将状态编码为固定 AT 文本，便于主机测试。`app_tasks` 负责把命令执行结果映射为 `OK` 或具体错误。

## 测试

- 主机测试覆盖新命令、严格格式、状态文本、默认状态和联锁状态机。
- Debug/Release 使用 Cube 附带 ARM GCC、CMake、Ninja 构建，App 代码保持 `-Wall -Wextra -Werror`。
- 通过 ST-Link 在 SWD under reset 模式烧录并验证。
- 使用已连接 485/UART1 测试电源命令、状态查询和 12V/NMOS、18V/PWM 联锁。
- PB3/PB12 默认高电平及 ON/OFF 翻转需要用调试器 GPIO 寄存器或测量工具确认；AT 状态只证明软件状态。

## 文档范围

简洁更新 `README.md`、`TESTING.md`、`docs/board2-at-uart-pwm.md` 和板级需求文档，只记录新增命令、默认状态、联锁和必要实测步骤。
