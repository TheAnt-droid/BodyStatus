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
#include <cpu.h>
#include <I2C_spo2.h>


CPU_INT08U spo2_read_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U *value);
CPU_INT08S spo2_check_connection(CPU_INT08U slave_addr);
void Check_Sensor(CPU_INT08U I2C_addr,CPU_INT08U sensor_reg,CPU_INT08U sensor_id);


/* [] END OF FILE */
