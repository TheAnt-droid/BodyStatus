/*
*********************************************************************************************************
*                              OXIMETER 5 CLICK (MAX30102) - CONFIGURATION
*
* Filename      : bsp_oximeter_cfg.h
*
* Description   : Register addresses, configuration values, limits of the heart rate calculation and the
*                 data types shared by bsp_spo2.c, bsp_spo2_calc.c, bsp_hr.c and app.c.
*********************************************************************************************************
*/
#ifndef BSP_OXIMETER_CFG_H
#define BSP_OXIMETER_CFG_H

#include <cpu.h>

/*---- Register addresses ----*/
#define FIFO_Config_Reg         0x08u   /* FIFO configuration                                           */
#define MODE_Config_Reg         0x09u   /* Mode configuration                                           */
#define SPO2_Config_Reg         0x0Au   /* SpO2 configuration                                           */
#define LED_PA_1_Config_Reg     0x0Cu   /* Current of the red LED                                       */
#define LED_PA_2_Config_Reg     0x0Du   /* Current of the IR LED                                        */

#define SPO2_ADDRESS            0x57u   /* I2C address of the MAX30102 (datasheet)                      */
#define SPO2_ID_REG             0xFFu   /* Register with the part ID                                    */

#define FIFO_DATA_REG           0x07u   /* Measured data                                                */
#define FIFO_RD_PTR_REG         0x06u   /* FIFO read index                                              */
#define FIFO_WR_PTR_REG         0x04u   /* FIFO write index                                             */
#define FIFO_OVF_COUNTER_REG    0x05u   /* FIFO overflow counter                                        */
#define FIFO_PTR_RESET          0x00u   /* Start value of read/write index and overflow counter         */

/*---- FIFO data format ----*/
#define FIFO_SAMPLE_BYTES       6u      /* One sample: 3 bytes red, then 3 bytes IR                     */
#define FIFO_RED_IDX            0u      /* Position of the red value in the sample                      */
#define FIFO_IR_IDX             3u      /* Position of the IR value in the sample                       */
#define FIFO_BYTE_MSB           0u      /* Byte order inside one value: MSB first                       */
#define FIFO_BYTE_MID           1u
#define FIFO_BYTE_LSB           2u
#define FIFO_MSB_MASK           0x03u   /* Only bits 1..0 of the MSB are data (18 bit value)            */
#define FIFO_MSB_SHIFT          16u     /* MSB -> bits 17..16                                           */
#define FIFO_MID_SHIFT          8u      /* Middle byte -> bits 15..8                                    */

/*---- Configuration values ----*/
#define FIFO_Config_Val         0x57u   /* 010 (sample avg: 4 samples) 1 (rollover) 0111 (FIFO full)    */
#define MODE_Config_Val         0x03u   /* 0 (shutdown) 0 (reset) 000 (reserved) 011 (SpO2) -> WAKE     */
#define MODE_Config_Val_Sleep   0x83u   /* 1 (shutdown) 0 (reset) 000 (reserved) 011 (SpO2) -> SLEEP    */
#define SPO2_Config_Val         0x2Bu   /* 0 (reserved) 01 (ADC range 4096) 010 (200 Hz) 11 (18 bit)    */
#define LED_PA_1_Config_Val     0x1Fu   /* 00011111: LED current 6.2 mA                                 */
#define LED_PA_2_Config_Val     0x1Fu   /* 00011111: LED current 6.2 mA                                 */

/*---- Other values ----*/
#define EFFECTIVE_SAMPLING_FREQ 50u     /* 200 Hz / 4 averaged samples. Change together with            */
                                        /* FIFO_Config_Val and SPO2_Config_Val!                         */
#define SPO2_ID_VAL             0x15u   /* Part ID of the MAX30102 (datasheet)                          */

#define FIFO_SAMPLE_BUFFER      32u     /* FIFO depth of the sensor, then it rolls over                 */
#define SAMPLE_BUFFER_SIZE      200u    /* Ring buffer of App_TaskSPO2CALC: 200 samples = 4 s at 50 Hz  */

/*---- Heart rate values ----*/
#define FINGER_THRESHOLD        5000u   /* Red and IR above this value: finger on the sensor            */
#define MIN_PEAK_DISTANCE       15      /* Min. samples between two peaks (0.3 s at 50 Hz)              */
#define PEAK_THRESHOLD          200     /* IR must fall this far below the maximum to count as a peak   */
#define LOW_HR_BORDER           30u     /* Plausible heart rate in bpm: lower limit                     */
#define HIGH_HR_BORDER          300u    /*                              upper limit                     */

#define NUM_PEAKS_FOR_HR (SAMPLE_BUFFER_SIZE / EFFECTIVE_SAMPLING_FREQ)    /* Min. peaks in the buffer  */
#define HR_MIN_SAMPLES          100u    /* Heart rate only with at least 100 samples (2 s)              */
#define HR_S_PER_MIN            60u     /* Seconds per minute: samples between peaks -> bpm             */

/*---- SpO2 calibration curve, see spo2_calc_percent() ----*/
#define SPO2_CAL_A              (-45.060f)  /* SpO2 = A * R^2 + B * R + C                               */
#define SPO2_CAL_B              30.354f
#define SPO2_CAL_C              94.845f

/*---- Structs ----*/
typedef struct
{
    CPU_INT32U red;
    CPU_INT32U ir;
} spo2;                                 /* One raw sample, App_TaskSPO2 -> App_TaskSPO2CALC */

typedef struct
{
    CPU_INT16U buffer_idx;
    CPU_INT16U samples_count;
    CPU_INT32U red[SAMPLE_BUFFER_SIZE];
    CPU_INT32U ir[SAMPLE_BUFFER_SIZE];
} spo2_buffer;                          /* Ring buffer of the last SAMPLE_BUFFER_SIZE samples */

typedef struct
{
    CPU_INT32U spo2;
    CPU_INT32U hr;
} oxi_results;                          /* Result of one second, App_TaskSPO2CALC -> App_TaskLED */

/*---- Error codes ----*/
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
