#ifndef __SCPI_CALLBACKS_H
#define __SCPI_CALLBACKS_H

#include <stdint.h>

#include "scpi_core.h"

/* 业务层输出接口：由任务层注入（例如通过 USB CDC 发送到 PC） */
typedef void (*SCPI_SendLineFn)(const char *line);
void SCPI_Callbacks_Init(SCPI_SendLineFn send_fn);

/* 采集触发：由任务层注入数据任务句柄（用于 :ACQ:START） */
void SCPI_SetDataTaskHandle(void *task_handle); /* 避免在头文件里强依赖 FreeRTOS 类型 */

/* 命令表导出给解析引擎使用 */
extern const SCPI_Entry_t scpi_commands[];
extern const uint32_t scpi_commands_count;

#endif /* __SCPI_CALLBACKS_H */


