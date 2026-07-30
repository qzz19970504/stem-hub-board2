#ifndef APP_TASKS_H
#define APP_TASKS_H

/** System task: establish safe outputs and indicate scheduler operation. */
void App_SystemTask(void *argument);

/** UART1 command parser and downstream forwarding task. */
void App_AtTask(void *argument);

/** UART2/UART3 upstream event task. */
void App_BridgeTask(void *argument);

#endif
