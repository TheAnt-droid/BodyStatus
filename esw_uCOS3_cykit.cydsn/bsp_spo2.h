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

#define FIFO_Reg 0x08u
#define MODE_Reg 0x09u
#define SPO2_Reg 0x0Au
#define LED_PA_1_Reg 0x0Cu
#define LED_PA_2_Reg 0x0Du

#define FIFO_Config 0x17u
#define MODE_Config 0x03u
#define SPO2_Config 0x23u
#define LED_PA_1_Config 0x1Fu
#define LED_PA_2_Config 0x1Fu

#define SPO2_ADDRESS 0x57u //MAX30102 address from datasheet
#define MAX_ID_REG 0xFFu //MAX30102 Register for ID
#define MAX_ID 0x15u //MAX30102 ID from datasheet

#define FIFO_SAMPLE_BUFFER 32u
#define FIFO_DATA 0x07u
#define FIFO_RD_PTR 0x06u
#define FIFO_WR_PTR 0x04u
#define FIFO_OVF_COUNTER 0x05u



CPU_INT08U spo2_read_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U *value);
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U config_value);

CPU_INT08S spo2_check_connection(CPU_INT08U slave_addr);
void Check_Sensor(CPU_INT08U I2C_addr,CPU_INT08U sensor_reg,CPU_INT08U sensor_id);
CPU_INT08U spo2_config(CPU_INT08U I2C_addr);

CPU_INT08U spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir);
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr,CPU_INT08U *num_samples_available);

/* [] END OF FILE */
