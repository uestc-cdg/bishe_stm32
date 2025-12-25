#ifndef __SCPI_CORE_H
#define __SCPI_CORE_H

#include <stdint.h>

/* 纯解析层：不依赖任何硬件/RTOS */
typedef void (*SCPI_Callback_t)(char *args);

typedef struct
{
    char *command;               /* 命令关键字，如 "*IDN?"、":FPGA:LED"、":ACQ:START" */
    SCPI_Callback_t callback;    /* 回调函数，args 指向命令后的参数字符串（可能为空） */
} SCPI_Entry_t;

void SCPI_Parse(char *input_string);

#endif /* __SCPI_CORE_H */


