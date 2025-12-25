/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
	#include <stdint.h>
	extern uint32_t SystemCoreClock;
#endif

//-----------------------------------------------------------------
// FreeRTOS基础配置选项
//-----------------------------------------------------------------
#define configUSE_PREEMPTION						1															// 1：使用抢占式内核，0：使用协程
#define configUSE_IDLE_HOOK							0															// 1：使用空闲钩子函数，0：不使用
#define configUSE_TICK_HOOK							0															// 1：使用时间片钩子函数，0：不使用
#define configCPU_CLOCK_HZ							( SystemCoreClock )						// CPU频率
#define configTICK_RATE_HZ							( ( TickType_t ) 1000 )				// 系统时钟节拍频率，设置为1000Hz，周期为1ms
#define configMAX_PRIORITIES						( 32 )												// 可使用的最大优先级数
#define configMINIMAL_STACK_SIZE				( ( unsigned short ) 130 )		// 空闲任务使用的堆栈大小
#define configTOTAL_HEAP_SIZE						( ( size_t ) ( 75 * 1024 ) )	// 系统所能使用的堆栈大小
#define configMAX_TASK_NAME_LEN					( 10 )												// 任务名字的字符串长度
#define configUSE_TRACE_FACILITY				1															// 1：启用可视化跟踪调试，0：不启用
#define configUSE_16_BIT_TICKS					0															// 滴答计数器变量数据类型。1：无符号16位类型，0：无符号32位类型
#define configIDLE_SHOULD_YIELD					1															// 1：空闲任务放弃CPU使用权，让给其他同优先级的用户任务
#define configUSE_MUTEXES								1															// 1：使用互斥信号量
#define configQUEUE_REGISTRY_SIZE				8															// 不为0时表示启用队列记录，具体的值是记录的队列和信号量最大数目
#define configCHECK_FOR_STACK_OVERFLOW	0															/* 大于0时启用堆栈溢出检测功能
																																				 如果使用此功能用户必须提供一个栈溢出钩子函数。
																																				 如果使用的话，此值可以为1或者2，因为有两种栈溢出检测方法 */
#define configUSE_RECURSIVE_MUTEXES			1															// 1：使用递归互斥信号量
#define configUSE_MALLOC_FAILED_HOOK		0															// 1：使用内存申请失败钩子函数
#define configUSE_APPLICATION_TASK_TAG	0															// 1：启用vTaskSetApplicationTaskTag()和xTaskCallApplicationTaskHook()API函数
#define configUSE_COUNTING_SEMAPHORES		1															// 1：使用计数信号量
#define configGENERATE_RUN_TIME_STATS		0															// 1：启用运行时间统计功能

//-----------------------------------------------------------------
// FreeRTOS与协程有关的配置选项
//-----------------------------------------------------------------
#define configUSE_CO_ROUTINES 					0															// 1：启用协程，启用协程后必须添加croutine.c文件
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )													// 协程的有效优先级数目

//-----------------------------------------------------------------
// FreeRTOS与软件定时器有关的配置选项
//-----------------------------------------------------------------
#define configUSE_TIMERS								1															// 1：启用软件定时器
#define configTIMER_TASK_PRIORITY				( configMAX_PRIORITIES-1 )		// 软件定时器优先级
#define configTIMER_QUEUE_LENGTH				10														// 软件定时器队列长度
#define configTIMER_TASK_STACK_DEPTH		( configMINIMAL_STACK_SIZE * 2 )	// 软件定时器任务堆栈大小

//-----------------------------------------------------------------
// FreeRTOS可选函数的配置选项
//-----------------------------------------------------------------
// 以下定义设置为1则可以使用对应的API函数，反之则不能使用
#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete							1
#define INCLUDE_vTaskCleanUpResources		1
#define INCLUDE_vTaskSuspend						1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay							1

//-----------------------------------------------------------------
// FreeRTOS与中断有关的配置选项
//-----------------------------------------------------------------
#ifdef __NVIC_PRIO_BITS
	/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
	#define configPRIO_BITS       		__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       		4        /* 15 priority levels */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			0xf			// 中断最低优先级

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	5			// 系统可管理的最高中断优先级

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )					// 设置中断内核优先级
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )		// 低于此优先级的可安全的调用FreeRTOS的API函数，高于则禁止
	
// 普通的assert（）语义，而无需依赖assert.h头文件的提供。
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }	
	
//-----------------------------------------------------------------
// FreeRTOS与中断服务函数有关的配置选项
//-----------------------------------------------------------------
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
//#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */

