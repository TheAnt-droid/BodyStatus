/*
*********************************************************************************************************
*                                      BSP - PWM FOR THE RGB LED (Click Shield)
*
* Filename      : bsp_pwm.c
*
* Description   : Start and duty cycle of the three PWMs of the RGB LED, see bsp_pwm.h.
*********************************************************************************************************
*/

#include <bsp_pwm.h>
#include <string.h>                                             /* strcmp()                             */

/*
*********************************************************************************************************
*                                             init_pwm()
*
* Description : Starts the three PWMs with the LED off.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : BSP_PostInit()
*
* Note(s)     : (1) The compare value 0 is also set in TopDesign. Setting it here as well means the LED
*                   is off after reset, independent of the TopDesign settings.
*********************************************************************************************************
*/

void init_pwm(void)
{
    PWM_R_Start();
    PWM_G_Start();
    PWM_B_Start();

    PWM_R_WriteCompare(PWM_DUTY_OFF);                           /* LED off after reset                  */
    PWM_G_WriteCompare(PWM_DUTY_OFF);
    PWM_B_WriteCompare(PWM_DUTY_OFF);
}

/*
*********************************************************************************************************
*                                          LED_off() / LED_on()
*
* Description : Switches the red or the green LED fully off / fully on.
*
* Argument(s) : color   "r" for red, "g" for green. Anything else is ignored.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskLED(), App_TaskLEDE_ENV()
*
* Note(s)     : (1) Blue is only set with LED_spo2().
*********************************************************************************************************
*/

void LED_off(const CPU_CHAR *color)
{
    if (strcmp(color, "r") == 0)
    {
        PWM_R_WriteCompare(PWM_DUTY_OFF);
    }
    else if (strcmp(color, "g") == 0)
    {
        PWM_G_WriteCompare(PWM_DUTY_OFF);
    }
}

void LED_on(const CPU_CHAR *color)
{
    if (strcmp(color, "r") == 0)
    {
        PWM_R_WriteCompare(PWM_DUTY_MAX);
    }
    else if (strcmp(color, "g") == 0)
    {
        PWM_G_WriteCompare(PWM_DUTY_MAX);
    }
}

/*
*********************************************************************************************************
*                                             LED_spo2()
*
* Description : Sets the brightness of the blue LED proportional to SpO2.
*
* Argument(s) : buffer_spo2     pointer to SpO2 in percent (0..PWM_PERCENT_MAX).
*
* Return(s)   : none
*
* Caller(s)   : App_TaskLED()
*
* Note(s)     : (1) The caller limits the value to PWM_PERCENT_MAX, otherwise the result would not fit
*                   into the 8 bit compare value.
*********************************************************************************************************
*/

void LED_spo2(CPU_INT32U *buffer_spo2)
{
    PWM_B_WriteCompare((CPU_INT08U)((*buffer_spo2 * PWM_DUTY_MAX) / PWM_PERCENT_MAX));
}

/* [] END OF FILE */
