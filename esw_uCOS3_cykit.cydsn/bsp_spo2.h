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
/*---- INCLUDES ----*/
#include <cpu.h>
#include <I2C_spo2.h>

/*---- Register Addresses ----*/
#define FIFO_Config_Reg 0x08u //Adress for configuring FIFO
#define MODE_Config_Reg 0x09u //Adress for configuring MODE
#define SPO2_Config_Reg 0x0Au //Adress for configuring SPO2
#define LED_PA_1_Config_Reg 0x0Cu //Adress for configuring RED LED
#define LED_PA_2_Config_Reg 0x0Du //Adress for configuring IR LED

#define SPO2_ADDRESS 0x57u //MAX30102 address from datasheet
#define SPO2_ID_REG 0xFFu //MAX30102 Register containing ID

#define FIFO_DATA_REG 0x07u //Adress where measured data is stored
#define FIFO_RD_PTR_REG 0x06u // Adress where read index is stored
#define FIFO_WR_PTR_REG 0x04u //Adress where write index is stored
#define FIFO_OVF_COUNTER_REG 0x05u //Adress where overflow counter is stored

/*---- Config Values ----*/
#define FIFO_Config_Val 0x17u //000(Sample AVG) 1(RollOver) 0111(FiFo Full Behaviour)
#define MODE_Config_Val 0x03u //0(Shutdown) 0(Reset) 000(Reserved) 011(Mode:SpO2)
#define SPO2_Config_Val 0x23u //0(Reserved) 01(ADC Range:4096) 000(Sample Rate:50Hz) 11(ADC Res:18bit)
#define LED_PA_1_Config_Val 0x1Fu //00011111(LED Current:6.2mA)
#define LED_PA_2_Config_Val 0x1Fu //00011111 (LED Current:6.2mA)

/*---- Other Values ----*/
#define SPO2_ID_VAL 0x15u //MAX30102 ID from datasheet

#define FIFO_SAMPLE_BUFFER 32u //This many samples get saved in the FIFO buffer before rollover.

/*---- Error Codes ----*/
typedef enum{
    SPO2_ERR_NONE=0,
    SPO2_ERR_SENSOR_READ,
    SPO2_ERR_MEMORY,
    SPO2_ERR_QUEUE_POST,
    SPO2_ERR_INVALID_SAMPLE,
    SPO2_ERR_VAL,
    SPO2_ERR_MASTER_SEND,
    SPO2_ERR_MASTER_WRITE,
    SPO2_ERR_MASTER_READ,
    SPO2_ERR_NO_CONNECTION,
}SPO2_ERROR;


CPU_INT08U spo2_read_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U *value);
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U config_value);

CPU_INT08U spo2_check_connection(CPU_INT08U slave_addr);
void Check_Sensor(CPU_INT08U slave_addr,CPU_INT08U sensor_reg,CPU_INT08U sensor_id,SPO2_ERROR *spo2_err);
void spo2_config(CPU_INT08U slave_addr,SPO2_ERROR *spo2_err);

void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir,SPO2_ERROR *spo2_err);
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr,CPU_INT08U *num_samples_available,SPO2_ERROR *spo2_err);

/* [] END OF FILE */
