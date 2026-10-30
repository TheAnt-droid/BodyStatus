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
#ifndef BSP_SPO2_H
#define BSP_SPO2_H
/*---- INCLUDES ----*/
#include <cpu.h>
#include <I2C_spo2.h>
#include <bsp_oximeter_cfg.h>

CPU_INT08U spo2_read_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U *value);
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U config_value);

CPU_INT08U spo2_check_connection(CPU_INT08U slave_addr);
void Check_Sensor(CPU_INT08U slave_addr, CPU_INT08U sensor_reg, CPU_INT08U sensor_id, SPO2_ERROR *spo2_err);
void spo2_config(CPU_INT08U slave_addr, SPO2_ERROR *spo2_err);

void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir, SPO2_ERROR *spo2_err);
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available, SPO2_ERROR *spo2_err);

#endif
/* [] END OF FILE */
