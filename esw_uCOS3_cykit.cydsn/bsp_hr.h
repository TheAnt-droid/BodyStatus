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
#ifndef  BSP_HR_H
#define  BSP_HR_H
    
#include <cpu.h>
#include <bsp_oximeter_cfg.h>
    
void calculate_heartrate(spo2_buffer *buffer, CPU_INT16U *heartrate);

#endif
/* [] END OF FILE */
