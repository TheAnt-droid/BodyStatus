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
#include <PWM_B.h>
#include <PWM_G.h>
#include <PWM_R.h>
#include <bsp_oximeter_cfg.h>
#include <os.h>

void init_pwm(void);
void LED_off (const char* color);
void LED_on (const char* color);
void LED_spo2(CPU_INT32U *buffer_spo2);

/* [] END OF FILE */
