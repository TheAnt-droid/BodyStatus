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
#ifndef BSP_OXIMETER_CFG_H
#define BSP_OXIMETER_CFG_H

#include <cpu.h>

/*---- Register Addresses ----*/
#define FIFO_Config_Reg 0x08u     // Adress for configuring FIFO
#define MODE_Config_Reg 0x09u     // Adress for configuring MODE 
#define SPO2_Config_Reg 0x0Au     // Adress for configuring SPO2
#define LED_PA_1_Config_Reg 0x0Cu // Adress for configuring RED LED
#define LED_PA_2_Config_Reg 0x0Du // Adress for configuring IR LED

#define SPO2_ADDRESS 0x57u // MAX30102 address from datasheet
#define SPO2_ID_REG 0xFFu  // MAX30102 Register containing ID

#define FIFO_DATA_REG 0x07u        // Adress where measured data is stored
#define FIFO_RD_PTR_REG 0x06u      // Adress where read index is stored
#define FIFO_WR_PTR_REG 0x04u      // Adress where write index is stored
#define FIFO_OVF_COUNTER_REG 0x05u // Adress where overflow counter is stored

/*---- Config Values ----*/
#define FIFO_Config_Val 0x57u     // 010(Sample AVG:4samples) 1(RollOver) 0111(FiFo Full Behaviour)
#define MODE_Config_Val 0x03u     // 0(Shutdown) 0(Reset) 000(Reserved) 011(Mode:SpO2) -> WAKE
#define MODE_Config_Val_Sleep 0x83u // 1(Shutdown) 0(Reset) 000(Reserved) 011(Mode:SpO2) -> WAKE
#define SPO2_Config_Val 0x2Bu     // 0(Reserved) 01(ADC Range:4096) 010(Sample Rate:200Hz) 11(ADC Res:18bit)
#define LED_PA_1_Config_Val 0x1Fu // 00011111(LED Current:6.2mA)
#define LED_PA_2_Config_Val 0x1Fu // 00011111 (LED Current:6.2mA)

/*---- Other Values ----*/
#define EFFECTIVE_SAMPLING_FREQ 50u // DEPENDS ON CONFIGURED FS AND SAMPLE AVG!
#define SPO2_ID_VAL 0x15u           // MAX30102 ID from datasheet

#define FIFO_SAMPLE_BUFFER 32u // This many samples get saved in the FIFO buffer before rollover.
#define SAMPLE_BUFFER_SIZE 200u
/*---- Heart Rate Values ----*/
#define FINGER_THRESHOLD 5000u
#define MIN_PEAK_DISTANCE 15
#define PEAK_THRESHOLD 200
#define LOW_HR_BORDER 30u
#define HIGH_HR_BORDER 300u

#define NUM_PEAKS_FOR_HR (SAMPLE_BUFFER_SIZE / EFFECTIVE_SAMPLING_FREQ)

/*---- Structs ----*/
typedef struct
{
    CPU_INT32U red;
    CPU_INT32U ir;
} spo2;

typedef struct
{
    CPU_INT32U spo2;
    CPU_INT32U hr;
} oxi_results;

typedef struct
{
    CPU_INT16U buffer_idx;
    CPU_INT16U samples_count;
    CPU_INT32U red[SAMPLE_BUFFER_SIZE];
    CPU_INT32U ir[SAMPLE_BUFFER_SIZE];
} spo2_buffer;

/*---- Error Codes ----*/
typedef enum
{
    SPO2_ERR_NONE = 0,
    SPO2_ERR_SENSOR_READ,
    SPO2_ERR_MEMORY,
    SPO2_ERR_QUEUE_POST,
    SPO2_ERR_INVALID_SAMPLE,
    SPO2_ERR_VAL,
    SPO2_ERR_MASTER_SEND,
    SPO2_ERR_MASTER_WRITE,
    SPO2_ERR_MASTER_READ,
    SPO2_ERR_NO_CONNECTION,
} SPO2_ERROR;

#endif
/* [] END OF FILE */
