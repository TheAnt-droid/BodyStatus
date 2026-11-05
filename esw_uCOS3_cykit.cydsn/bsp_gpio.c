/*
*********************************************************************************************************
*                                         BSP - GPIO (Click Shield)
*
* Filename      : bsp_gpio.c
*
* Description   : Push button SW1 of the Click Shield (P2_2, switch S1 on "BTN").
*********************************************************************************************************
*/

#include <bsp_gpio.h>

/*
*********************************************************************************************************
*                                             init_gpio()
*
* Description : Configures the push button pin as input with internal pull-up.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_PostInit().
*
* Note(s)     : (1) SW1 connects the pin directly to GND, the shield has no pull-up resistor. With the
*                   resistive pull-up the pin reads high when released and low when pressed. (A strong
*                   high output would be short-circuited to GND on every press.)
*               (2) The same drive mode is set in TopDesign; set here again so the start state is
*                   defined in code too.
*********************************************************************************************************
*/

CPU_VOID init_gpio(CPU_VOID)
{
    push_button_SetDriveMode(push_button_DM_RES_UP);            /* Input with pull-up (Note 1)          */
    push_button_Write(GPIO_PB_RELEASED);                        /* Data register 1 = pull-up active     */
}

/*
*********************************************************************************************************
*                                             gpio_read()
*
* Description : Reads the level of a GPIO pin.
*
* Argument(s) : port    port identifier (only PORT2).
*
*               pin     pin identifier (only P2_2).
*
* Return(s)   : 0 / 1           pin level (push button: 0 = pressed, 1 = released).
*               GPIO_READ_ERR   unknown port or pin.
*
* Caller(s)   : BSP_PB_StatusGet().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08S gpio_read(CPU_INT08U port, CPU_INT08U pin)
{
    CPU_INT08S result;


    if ((port == PORT2) && (pin == P2_2)) {
        result = (CPU_INT08S)push_button_Read();
    } else {
        result = GPIO_READ_ERR;
    }
    return (result);
}

/* [] END OF FILE */
