/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include <cpu.h>

#include <UART_1.h>

CPU_VOID init_uart(CPU_VOID);
CPU_INT08U uart_get_byte(CPU_VOID);
CPU_VOID uart_send_byte(CPU_INT08U byte);
void uart_send_string(CPU_INT08U *str);
void uint32_to_str(CPU_INT32U val, char *p_buf);
void float_to_str(float val, char *p_buf, int decimal_places);

/* [] END OF FILE */
