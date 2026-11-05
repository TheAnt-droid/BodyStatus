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
#ifndef BSP_SPO2_CALC_H
#define BSP_SPO2_CALC_H

#include <cpu.h>
#include <bsp_oximeter_cfg.h>

void spo2_add_sample(CPU_INT32U red, CPU_INT32U ir, spo2_buffer *buffer);
CPU_BOOLEAN spo2_calc_dc(spo2_buffer *buffer, CPU_INT32U *avg_red, CPU_INT32U *avg_ir);
void spo2_calc_ac(spo2_buffer *buffer, CPU_INT32U *ac_red, CPU_INT32U *ac_ir);
void spo2_calc_ratio(CPU_INT32U dc_red, CPU_INT32U dc_ir, CPU_INT32U ac_red, CPU_INT32U ac_ir, float *ratio);
void spo2_calc_percent(float ratio, CPU_INT08U *spo2);

#endif
/* [] END OF FILE */
