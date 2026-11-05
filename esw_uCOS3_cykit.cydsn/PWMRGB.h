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
void init_pwm(void);
void LED_off (const char* color);
void LED_on (const char* color);
void OximterData(oxi_results *oximeter, OS_ERR *p_err, CPU_INT32U *hr_ptr,  CPU_INT32U *spo2_ptr);
/* [] END OF FILE */
