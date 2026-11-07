/*
*********************************************************************************************************
*                                      BSP - PWM FOR THE RGB LED (Click Shield)
*
* Filename      : bsp_pwm.h
*
* Description   : Wrapper around the PSoC PWM components PWM_R, PWM_G and PWM_B (8 bit, period 255,
*                 common clock Clock_1). Only this file talks to the generated PWM APIs, the LED tasks
*                 in app.c use the functions below.
*
* Pins (Click Shield RGB LED):
*                 red P2_7, green P1_2, blue P2_1
*********************************************************************************************************
*/

#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <cpu.h>
#include <PWM_R.h>
#include <PWM_G.h>
#include <PWM_B.h>

/*
*********************************************************************************************************
*                                              DEFINES
*********************************************************************************************************
*/

#define PWM_DUTY_OFF            0u      /* Compare value 0: LED off (also the start value in TopDesign) */
#define PWM_DUTY_MAX            255u    /* Compare value = period: LED fully on                        */
#define PWM_PERCENT_MAX         100u    /* LED_spo2(): 100 % = PWM_DUTY_MAX                            */

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void init_pwm (void);
void LED_off  (const char *color);
void LED_on   (const char *color);
void LED_spo2 (CPU_INT32U *buffer_spo2);

#endif
/* [] END OF FILE */
