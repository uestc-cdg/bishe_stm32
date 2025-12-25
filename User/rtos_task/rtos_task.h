#ifndef __RTOS_TASK_H
#define __RTOS_TASK_H

#include <stdint.h>

/* 线程安全的USB发送函数 */
void USB_Send_Safe(uint8_t *buf, uint32_t len);

void RTOS_Task_Init(void);

#endif

