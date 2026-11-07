/*
*********************************************************************************************************
*                                 BSP - UART (debug output via KitProg)
*
* Filename      : bsp_uart.h
*
* Description   : Wrapper around the PSoC UART component UART_1, see bsp_uart.c.
*********************************************************************************************************
*/

#ifndef BSP_UART_H
#define BSP_UART_H

#include <cpu.h>

#include <UART_1.h>

CPU_VOID init_uart(CPU_VOID);
CPU_INT08U uart_get_byte(CPU_VOID);
CPU_VOID uart_send_byte(CPU_INT08U byte);
void uart_send_string(CPU_INT08U *str);

#endif

/* [] END OF FILE */
