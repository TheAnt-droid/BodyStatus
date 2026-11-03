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
/*
*********************************************************************************************************
*                                       uint32_to_str()
*
* Description : Converts a uint32 datatype to a string
*
* Argument(s) : val ... containing the uint32 value
                *p_buf ... containing a pointer to a buffer array
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void uint32_to_str(CPU_INT32U val, char *p_buf)
{
    char temp[12];
    int i = 0, j = 0;

    if (val == 0)
    {
        p_buf[0] = '0';
        p_buf[1] = '\0';
        return;
    }

    while (val > 0)
    {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }

    // Reverse into output buffer
    while (i > 0)
    {
        p_buf[j++] = temp[--i];
    }
    p_buf[j] = '\0';
}

/*
*********************************************************************************************************
*                                       float_to_str()
*
* Description : Converts a float datatype to a string
*
* Argument(s) : val ... containing the float value
                *p_buf ... containing a pointer to a buffer array
                decimal_places ... how many decimal places are needed (ex. 2 -> 0.01)
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void float_to_str(float val, char *p_buf, int decimal_places)
{
    int j = 0;

    //Handle negative numbers
    if (val < 0.0f)
    {
        p_buf[j++] = '-';
        val = -val;
    }

    //Extract integer part
    long int_part = (long)val;

    //Extract and round fractional part
    float remainder = val - (float)int_part;
    long mult = 1;
    for (int p = 0; p < decimal_places; p++)
    {
        mult *= 10;
    }
    long frac_part = (long)(remainder * (float)mult + 0.5f); // Round up

    // Handle carry-over if rounding rolls over the fractional part
    if (frac_part >= mult)
    {
        int_part += 1;
        frac_part = 0;
    }

    //Convert integer part to string (into a temporary stack buffer)
    char temp[12];
    int i = 0;
    if (int_part == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        long temp_val = int_part;
        while (temp_val > 0)
        {
            temp[i++] = (temp_val % 10) + '0';
            temp_val /= 10;
        }
    }

    // Copy reversed integer part to output buffer
    while (i > 0)
    {
        p_buf[j++] = temp[--i];
    }

    //Append decimal point and fractional part if requested
    if (decimal_places > 0)
    {
        p_buf[j++] = '.';

        char frac_temp[10];
        int frac_i = 0;

        for (int p = 0; p < decimal_places; p++)
        {
            frac_temp[frac_i++] = (frac_part % 10) + '0';
            frac_part /= 10;
        }

        // Copy reversed fractional part to output buffer
        while (frac_i > 0)
        {
            p_buf[j++] = frac_temp[--frac_i];
        }
    }

    // Null-terminate the final string
    p_buf[j] = '\0';
}
/* [] END OF FILE */
