/*
*********************************************************************************************************
*                                  OXIMETER 5 CLICK (MAX30102) - DRIVER
*
* Filename      : bsp_spo2.h
*
* Description   : I2C access to the MAX30102 (I2C_spo2, mikroBUS slot 2), see bsp_spo2.c.
*********************************************************************************************************
*/
#ifndef BSP_SPO2_H
#define BSP_SPO2_H
    
/*---- INCLUDES ----*/
#include <cpu.h>
#include <I2C_spo2.h>
#include <bsp_oximeter_cfg.h>

CPU_INT08U spo2_read_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U *value);
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U config_value);


void Check_Sensor(CPU_INT08U slave_addr, CPU_INT08U sensor_reg, CPU_INT08U sensor_id, SPO2_ERROR *spo2_err);
void spo2_config(CPU_INT08U slave_addr, SPO2_ERROR *spo2_err);

void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir, SPO2_ERROR *spo2_err);
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available, SPO2_ERROR *spo2_err);
SPO2_ERROR spo2_sleep(void);
SPO2_ERROR spo2_wake(void);

#endif
/* [] END OF FILE */
