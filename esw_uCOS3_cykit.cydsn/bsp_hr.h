/*
*********************************************************************************************************
*                                OXIMETER 5 CLICK (MAX30102) - HEART RATE
*
* Filename      : bsp_hr.h
*
* Description   : Heart rate from the IR samples in the ring buffer, see bsp_hr.c.
*********************************************************************************************************
*/
#ifndef  BSP_HR_H
#define  BSP_HR_H
    
#include <cpu.h>
#include <bsp_oximeter_cfg.h>
    
void calculate_heartrate(spo2_buffer *buffer, CPU_INT16U *heartrate);

#endif
/* [] END OF FILE */
