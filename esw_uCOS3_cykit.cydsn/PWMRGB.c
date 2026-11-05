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

#include <PWMRGB.h>

void init_pwm(void)
{
  PWM_B_Start();
  PWM_G_Start();
  PWM_R_Start();
}

static void OximeterData(oxi_results *oximeter, OS_ERR *p_err, CPU_INT32U *hr_ptr,  CPU_INT32U *spo2_ptr);
{
  if(*p_err==OS_ERR_NONE && oximeter !=NULL){
    *spo2_ptr = oximeter->spo2;
    *hr_ptr = oximter-> hr;
    
    OSMemPut(&SpO2Mem, (void *)oximeter, p_err);
    
    CPU_INT32U buffer_spo2=(*spo2_ptr >100)?1ßß : *spo2_ptr;
    PWM_B_WriteCompare((CPU_INT08U)((buffer_spo2*255)/100));
  }
}

/* [] END OF FILE */
