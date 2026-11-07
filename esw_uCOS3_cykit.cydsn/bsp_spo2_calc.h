/*
*********************************************************************************************************
*                             OXIMETER 5 CLICK (MAX30102) - SPO2 CALCULATION
*
* Filename      : bsp_spo2_calc.h
*
* Description   : Ring buffer for the samples and SpO2 from its DC and AC parts, see bsp_spo2_calc.c.
*********************************************************************************************************
*/
#ifndef BSP_SPO2_CALC_H
#define BSP_SPO2_CALC_H

#include <cpu.h>
#include <lib_def.h>
#include <bsp_oximeter_cfg.h>

CPU_INT16U spo2_get_chronological_index(const spo2_buffer *buffer, CPU_INT16U idx);
void spo2_add_sample(CPU_INT32U red, CPU_INT32U ir, spo2_buffer *buffer);
CPU_BOOLEAN spo2_calc_dc(spo2_buffer *buffer, CPU_INT32U *avg_red, CPU_INT32U *avg_ir);
void spo2_calc_ac(spo2_buffer *buffer, CPU_INT32U *ac_red, CPU_INT32U *ac_ir);
void spo2_calc_ratio(CPU_INT32U dc_red, CPU_INT32U dc_ir, CPU_INT32U ac_red, CPU_INT32U ac_ir, CPU_FP32 *ratio);
void spo2_calc_percent(CPU_FP32 ratio, CPU_INT08U *spo2);

#endif
/* [] END OF FILE */
