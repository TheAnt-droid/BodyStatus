/*
*********************************************************************************************************
*                                         BSP - GPIO (Click Shield)
*
* Filename      : bsp_gpio.h
*
* Description   : Push button SW1 of the Click Shield (P2_2, switch S1 on "BTN").
*                 The RGB LED is driven by PWM (bsp_pwm), not by GPIO.
*********************************************************************************************************
*/

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <cpu.h>
#include <push_button.h>

/*
*********************************************************************************************************
*                                              DEFINES
*********************************************************************************************************
*/

#define PORT2               (1u << 1)   /* Port identifier for gpio_read()                             */
#define P2_2                (1u << 2)   /* Push button SW1 (INT1 of slot 1)                            */

#define GPIO_PB_RELEASED    1u          /* Pull-up: released = high, pressed = low (SW1 to GND)        */
#define GPIO_READ_ERR       (-1)        /* Unknown port / pin                                          */

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_VOID   init_gpio  (CPU_VOID);
CPU_INT08S gpio_read  (CPU_INT08U port, CPU_INT08U pin);

#endif
/* [] END OF FILE */
