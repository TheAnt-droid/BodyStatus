/*
*********************************************************************************************************
*                                 BSP - UART (debug output via KitProg)
*
* Filename      : bsp_uart.c
*
* Description   : Wrapper around the PSoC UART component UART_1 (115200 8N1). Rx P12_6 and Tx P12_7 go to
*                 the USB-UART bridge of the KitProg. In app.c only App_UartPrint() uses it (mutex).
*********************************************************************************************************
*/

#include <bsp_uart.h>

/*
*********************************************************************************************************
*                                       init_uart()
*
* Description : This function initializes the UART_1 communication module. Configuration: 115200,8N1
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/

CPU_VOID init_uart(CPU_VOID)
{
    UART_1_Start();
}

/*
*********************************************************************************************************
*                                       uart_get_byte()
*
* Description : Returns the last received byte of data. UART_1_GetChar() is
*               designed for ASCII characters and returns a uint8 where 1 to 255 are values
*               for valid characters and 0 indicates an error occurred or no data is present 
*
* Argument(s) : none.
*
* Return(s)   : Character read from UART RX buffer. ASCII characters from 1 to 255 are valid.
*               A returned zero signifies an error condition or no data available.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant, non-blocking.
*********************************************************************************************************
*/

CPU_INT08U uart_get_byte(CPU_VOID)
{
    return UART_1_GetChar();
}

/*
*********************************************************************************************************
*                                       uart_send_byte()
*
* Description : Puts a byte of data into the transmit buffer to be sent when the bus is
*               available. This is a blocking API that waits until the TX buffer has room to
*               hold the data. 
*
* Argument(s) : byte ... containing the data to transmit
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/

CPU_VOID uart_send_byte(CPU_INT08U byte)
{
    UART_1_PutChar(byte);
}

/*
*********************************************************************************************************
*                                       uart_send_string()
*
* Description : Sends a whole string at once. 
*
* Argument(s) : *str ... containing a pointer to the string 
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void uart_send_string(CPU_INT08U *str)
{
    while (*str != '\0')
    {
        UART_1_PutChar(*str);
        str++;
    }
}
/* [] END OF FILE */
