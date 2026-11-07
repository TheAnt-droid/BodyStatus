/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                              (c) Copyright 2012; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      APPLICATION CONFIGURATION
*
*                                           CYPRESS PSoC5
*                                              with the
*                                     CY8CKIT-059 Development Kit
*
* Filename      : app_cfg.h
* Version       : V1.00
* Programmer(s) : DC
*********************************************************************************************************
*/

#ifndef APP_CFG_MODULE_PRESENT
#define APP_CFG_MODULE_PRESENT

#include <stdio.h>

/*
*********************************************************************************************************
*                                       ADDITIONAL uC/MODULE ENABLES
*********************************************************************************************************
*/

#define APP_CFG_PROBE_COM_EN DEF_ENABLED

/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*
* Note(s) : (1) Priority plan with OS_CFG_PRIO_MAX = 16 (os_cfg.h), lower number = higher priority:
*                    0      reserved: ISR handler task (handbook appendix B)
*                    1      tick task       (os_cfg_app.h)
*                    2      timer task      (os_cfg_app.h)
*                    3      start task      - only initialisation, then sleeps
*                    4      button task     - short, must react immediately
*                    5, 6   SpO2 tasks      - reading the sensor FIFO is time critical
*                    7      LED task        - red/blue, heartbeat pulse must stay in time
*                    8      LED_ENV task    - green, reads the flags every 20 ms
*                    9      environment     - measures every 3 s, not time critical
*                    10..13 free
*                    14     statistic task  (os_cfg_app.h, OS_CFG_PRIO_MAX-2 as recommended in appendix B)
*                    15     idle task       (OS_CFG_PRIO_MAX-1, set by the kernel)
*********************************************************************************************************
*/

#define APP_CFG_TASK_START_PRIO 3u
#define APP_CFG_TASK_BUTTON_PRIO 4u
#define APP_CFG_TASK_SPO2_PRIO 5u
#define APP_CFG_TASK_SPO2CALC_PRIO 6u
#define APP_CFG_TASK_LED_PRIO 7u
#define APP_CFG_TASK_LEDE_PRIO 8u
#define APP_CFG_TASK_ENV_PRIO 9u

/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*                             Size of the task stacks (# of CPU_STK entries)
*********************************************************************************************************
*/

#define APP_CFG_TASK_START_STK_SIZE 128u
#define APP_CFG_TASK_BUTTON_STK_SIZE 64u
#define APP_CFG_TASK_SPO2_STK_SIZE 128u
#define APP_CFG_TASK_SPO2CALC_STK_SIZE 1024u                    /* local spo2_buffer alone is 402 entries */
#define APP_CFG_TASK_LED_STK_SIZE 128u
#define APP_CFG_TASK_LEDE_STK_SIZE 64u
#define APP_CFG_TASK_ENV_STK_SIZE 256u


/*
*********************************************************************************************************
*                                          TASK STACK SIZES LIMIT
*********************************************************************************************************
*/

#define APP_CFG_TASK_START_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_START_STK_SIZE_LIMIT (APP_CFG_TASK_START_STK_SIZE * (100u - APP_CFG_TASK_START_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_LED_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_LED_STK_SIZE_LIMIT (APP_CFG_TASK_LED_STK_SIZE * (100u - APP_CFG_TASK_LED_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_LEDE_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_LEDE_STK_SIZE_LIMIT (APP_CFG_TASK_LEDE_STK_SIZE * (100u - APP_CFG_TASK_LEDE_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_BUTTON_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_BUTTON_STK_SIZE_LIMIT (APP_CFG_TASK_BUTTON_STK_SIZE * (100u - APP_CFG_TASK_BUTTON_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_SPO2_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_SPO2_STK_SIZE_LIMIT (APP_CFG_TASK_SPO2_STK_SIZE * (100u - APP_CFG_TASK_SPO2_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_SPO2CALC_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_SPO2CALC_STK_SIZE_LIMIT (APP_CFG_TASK_SPO2CALC_STK_SIZE * (100u - APP_CFG_TASK_SPO2CALC_STK_SIZE_PCT_FULL)) / 100u

#define APP_CFG_TASK_ENV_STK_SIZE_PCT_FULL 90u
#define APP_CFG_TASK_ENV_STK_SIZE_LIMIT (APP_CFG_TASK_ENV_STK_SIZE * (100u - APP_CFG_TASK_ENV_STK_SIZE_PCT_FULL)) / 100u


/*
*********************************************************************************************************
*                                          uC/LIB CONFIGURATION
*********************************************************************************************************
*/
#define LIB_MEM_CFG_ARG_CHK_EXT_EN DEF_DISABLED
#define LIB_MEM_CFG_OPTIMIZE_ASM_EN DEF_DISABLED
#define LIB_MEM_CFG_ALLOC_EN DEF_DISABLED
#define LIB_MEM_CFG_HEAP_SIZE 1024u
#define LIB_STR_CFG_FP_EN DEF_DISABLED

/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#define TRACE_LEVEL_OFF 0
#define TRACE_LEVEL_INFO 1
#define TRACE_LEVEL_DEBUG 2

#define APP_TRACE_LEVEL TRACE_LEVEL_OFF
#define APP_TRACE printf

#define APP_TRACE_INFO(x) ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO) ? (void)(APP_TRACE x) : (void)0)
#define APP_TRACE_DEBUG(x) ((APP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG) ? (void)(APP_TRACE x) : (void)0)

#endif
