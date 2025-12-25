#include "scpi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "key.h"
#include "led.h"
#include "usbd_cdc_vcp.h"
#include "bsp_fpga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 任务句柄
TaskHandle_t SCPITask_Handler;

// FPGA 控制命令队列
#define FPGA_CMD_QUEUE_SIZE  10

// FPGA 命令类型
typedef enum
{
    FPGA_CMD_WRITE = 0,  // 写入命令
    FPGA_CMD_READ        // 读取命令
} FPGA_CmdType_t;

// FPGA 命令结构体
typedef struct
{
    FPGA_CmdType_t cmd_type;
    uint16_t reg_index;
    uint16_t write_value;  // 写入时使用
} FPGA_Cmd_t;

// FPGA 读取结果结构体
typedef struct
{
    uint16_t reg_index;
    uint16_t read_result;
} FPGA_ReadResult_t;

// 全局队列句柄（在rtos_task.c中定义）
extern QueueHandle_t xFPGACmdQueue;      // 命令队列（SCPI -> FPGA任务）
extern QueueHandle_t xFPGAResultQueue;  // 结果队列（FPGA任务 -> SCPI任务）

// 任务函数声明
void SCPI_Task(void *pvParameters);

/**
 * @brief  SCPI 任务初始化
 * @param  None
 * @retval None
 */
void SCPI_Task_Init(void)
{
    xTaskCreate((TaskFunction_t)SCPI_Task,
                (const char *)"SCPI_Task",
                (uint16_t)1024,
                (void *)NULL,
                (UBaseType_t)(tskIDLE_PRIORITY + 3),
                (TaskHandle_t *)&SCPITask_Handler);
}

/**
 * @brief  SCPI 任务主体
 * @param  pvParameters: 传入参数
 * @retval None
 */
void SCPI_Task(void *pvParameters)
{
    u8 key;
    u16 len;
    // u16 t;
    int num;
    int mod;
    char temp_buf[32];
    TickType_t last_key_time = 0;

    // 初始化 FPGA 接口
    BSP_FPGA_Init();

    while(1)
    {
        // 1. 按键扫描
        key = KEY_Scan(0);
        if(key)
        {
            // 软件防抖：检测到按键后，如果在 200ms 内再次触发，则忽略
            // 解决按键抖动导致的一次按下触发多次的问题
            if ((xTaskGetTickCount() - last_key_time) > 200) 
            {
                usb_printf("Key Pressed: %d\r\n", key);
                last_key_time = xTaskGetTickCount();
            }
        }

        // 2. 检查 USB 接收状态
        if(USB_USART_RX_STA & 0x8000)
        {
            len = USB_USART_RX_STA & 0x3FFF;
            
            // 临时 buffer 确保安全性
            if(len > 31) len = 31;
            memcpy(temp_buf, USB_USART_RX_BUF, len);
            temp_buf[len] = '\0'; // 添加结束符

            // 回显收到的数据（保留原有功能）
            usb_printf("Echo: %s\r\n", temp_buf);

            // SCPI 指令解析（PC-to-FPGA Control）
            // 1. 查询 FPGA ID 命令 :FPGA:ID?
            if(strcmp(temp_buf, ":FPGA:ID?") == 0 || strcmp(temp_buf, ":fpga:id?") == 0)
            {
                FPGA_Cmd_t cmd;
                FPGA_ReadResult_t result;
                cmd.cmd_type = FPGA_CMD_READ;
                cmd.reg_index = 0; // FPGA ID is at register offset 0
                
                // 发送读取命令到FPGA控制任务
                if(xFPGACmdQueue != NULL && xFPGAResultQueue != NULL)
                {
                    if(xQueueSend(xFPGACmdQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        // 等待FPGA任务完成读取（通过结果队列接收）
                        if(xQueueReceive(xFPGAResultQueue, &result, pdMS_TO_TICKS(200)) == pdTRUE)
                        {
                            if(result.reg_index == 0)
                            {
                                // 返回HEX格式的ID值
                                usb_printf("FPGA_ID:0x%04X\r\n", result.read_result);
                            }
                        }
                        else
                        {
                            usb_printf("ERROR:FPGA_READ_TIMEOUT\r\n");
                        }
                    }
                    else
                    {
                        usb_printf("ERROR:QUEUE_FULL\r\n");
                    }
                }
                else
                {
                    // 队列未初始化，直接读取（降级处理）
                    uint16_t fpga_id = BSP_FPGA_Read(0);
                    usb_printf("FPGA_ID:0x%04X\r\n", fpga_id);
                }
            }
            // 2. 查询命令 :FPGA:LED?
            else if(strcmp(temp_buf, ":FPGA:LED?") == 0)
            {
                FPGA_Cmd_t cmd;
                FPGA_ReadResult_t result;
                cmd.cmd_type = FPGA_CMD_READ;
                cmd.reg_index = FPGA_REG_LED_CTRL;
                
                // 发送读取命令到FPGA控制任务
                if(xFPGACmdQueue != NULL && xFPGAResultQueue != NULL)
                {
                    if(xQueueSend(xFPGACmdQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        // 等待FPGA任务完成读取（通过结果队列接收）
                        if(xQueueReceive(xFPGAResultQueue, &result, pdMS_TO_TICKS(200)) == pdTRUE)
                        {
                            if(result.reg_index == FPGA_REG_LED_CTRL)
                            {
                                usb_printf("FPGA_LED:%d\r\n", result.read_result);
                            }
                        }
                        else
                        {
                            usb_printf("ERROR:FPGA_READ_TIMEOUT\r\n");
                        }
                    }
                    else
                    {
                        usb_printf("ERROR:QUEUE_FULL\r\n");
                    }
                }
                else
                {
                    // 队列未初始化，直接读取（降级处理）
                    uint16_t fpga_val = BSP_FPGA_Read(FPGA_REG_LED_CTRL);
                    usb_printf("FPGA_LED:%d\r\n", fpga_val);
                }
            }
            // 3. 设置命令 :FPGA:LED <val>
            else if(strncmp(temp_buf, ":FPGA:LED", 9) == 0)
            {
                int fpga_val = 0;
                FPGA_Cmd_t cmd;
                char* p_val;
                
                p_val = strchr(temp_buf, ' '); // 查找空格
                if(p_val != NULL)
                {
                    fpga_val = atoi(p_val + 1); // 转换空格后的数值
                    // 限制值范围 0-7 (3位掩码: Bit0=Red, Bit1=Green, Bit2=Blue)
                    if(fpga_val < 0) fpga_val = 0;
                    if(fpga_val > 7) fpga_val = 7;
                }
                
                cmd.cmd_type = FPGA_CMD_WRITE;
                cmd.reg_index = FPGA_REG_LED_CTRL;
                cmd.write_value = (uint16_t)fpga_val;
                
                // 发送写入命令到FPGA控制任务
                if(xFPGACmdQueue != NULL)
                {
                    if(xQueueSend(xFPGACmdQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        usb_printf("OK\r\n");
                    }
                    else
                    {
                        usb_printf("ERROR:QUEUE_FULL\r\n");
                    }
                }
                else
                {
                    // 队列未初始化，直接写入（降级处理）
                    BSP_FPGA_Write(FPGA_REG_LED_CTRL, (uint16_t)fpga_val);
                    usb_printf("OK\r\n");
                }
            }
            else
            {
                // 3. 之前的简单数字逻辑 (STM32 LED)
                // 只有当不是 SCPI 指令时才执行此逻辑，避免冲突
                num = atoi(temp_buf);
                mod = num % 3;

                // usb_printf("STM32 LED Ctrl: %d, Mod 3: %d\r\n", num, mod);

                // 先熄灭所有灯 (高电平灭)
                LED_R = 1; 
                LED_G = 1; 
                LED_B = 1;

                switch(mod)
                {
                    case 0:
                        LED_B = 0; // 亮蓝灯
                        // usb_printf("Blue LED On\r\n");
                        break;
                    case 1:
                        LED_G = 0; // 亮绿灯
                        // usb_printf("Green LED On\r\n");
                        break;
                    case 2:
                        LED_R = 0; // 亮红灯
                        // usb_printf("Red LED On\r\n");
                        break;
                }
            }

            USB_USART_RX_STA = 0;
        }

        vTaskDelay(10); // 10ms 周期
    }
}
