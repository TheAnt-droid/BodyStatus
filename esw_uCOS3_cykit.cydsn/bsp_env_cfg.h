/*
*********************************************************************************************************
*                                  ENVIRONMENT 3 CLICK (BME688) - CONFIGURATION
*
* Filename      : bsp_env_cfg.h
*
* Description   : Register addresses, settings, data types and error codes of the BME688.
*                 Source: Bosch Sensortec, BME688 Datasheet BST-BME688-DS000-00 Rev. 1.0 (04/2021).
*
* Note(s)       : (1) Register addresses are the full 8-bit addresses of table 22. Via SPI only 7 address
*                     bits are sent; bit 7 of the full address selects the memory page (table 21):
*                         0x80..0xFF -> SPI page 0,   0x00..0x7F -> SPI page 1
*                     bsp_env.c switches the page automatically.
*********************************************************************************************************
*/

#ifndef BSP_ENV_CFG_H
#define BSP_ENV_CFG_H

#include <cpu.h>

/*
*********************************************************************************************************
*                                       SPI PROTOCOL (ch. 6.3)
*********************************************************************************************************
*/

#define ENV_SPI_RW_READ         0x80u   /* Control byte bit 7: 1 = read, 0 = write                     */
#define ENV_SPI_ADDR_MASK       0x7Fu   /* Only 7 address bits are sent                                */
#define ENV_REG_PAGE_SEL_BIT    0x80u   /* Full address bit 7: 1 -> page 0, 0 -> page 1 (Note 1)       */

/*
*********************************************************************************************************
*                                    ID AND STATUS REGISTERS (table 22)
*********************************************************************************************************
*/

#define ENV_REG_STATUS          0x73u   /* Holds the page bit, reachable from both pages               */
#define ENV_REG_CHIP_ID         0xD0u
#define ENV_REG_VARIANT_ID      0xF0u

#define ENV_STATUS_MEM_PAGE     0x10u   /* status<4> = spi_mem_page: 0 = page 0, 1 = page 1            */
#define ENV_CHIP_ID_VAL         0x61u   /* Expected chip id                                            */
#define ENV_VARIANT_ID_VAL      0x01u   /* Expected variant id: BME688 (ch. 5.3.2.1)                   */

/*
*********************************************************************************************************
*                               CALIBRATION REGISTERS (tables 13 to 16)
*
* Note(s) : (1) The calibration values lie in three register blocks; each block is read at once.
*           (2) 16 bit values: value = (MSB << 8) | LSB.
*           (3) par_h1 / par_h2 are 12 bit: 8 high bits in 0xE3 / 0xE1, the 4 low bits share 0xE2.
*********************************************************************************************************
*/

                                        /* ---- Block 1: 0x8A ... 0xA0 (page 0) ---------------------- */
#define ENV_CALIB1_START        0x8Au
#define ENV_CALIB1_LEN          23u
#define ENV_REG_PAR_T2_LSB      0x8Au
#define ENV_REG_PAR_T2_MSB      0x8Bu
#define ENV_REG_PAR_T3          0x8Cu
#define ENV_REG_PAR_P1_LSB      0x8Eu
#define ENV_REG_PAR_P1_MSB      0x8Fu
#define ENV_REG_PAR_P2_LSB      0x90u
#define ENV_REG_PAR_P2_MSB      0x91u
#define ENV_REG_PAR_P3          0x92u
#define ENV_REG_PAR_P4_LSB      0x94u
#define ENV_REG_PAR_P4_MSB      0x95u
#define ENV_REG_PAR_P5_LSB      0x96u
#define ENV_REG_PAR_P5_MSB      0x97u
#define ENV_REG_PAR_P7          0x98u
#define ENV_REG_PAR_P6          0x99u
#define ENV_REG_PAR_P8_LSB      0x9Cu
#define ENV_REG_PAR_P8_MSB      0x9Du
#define ENV_REG_PAR_P9_LSB      0x9Eu
#define ENV_REG_PAR_P9_MSB      0x9Fu
#define ENV_REG_PAR_P10         0xA0u

                                        /* ---- Block 2: 0xE1 ... 0xEE (page 0) ---------------------- */
#define ENV_CALIB2_START        0xE1u
#define ENV_CALIB2_LEN          14u
#define ENV_REG_PAR_H2_MSB      0xE1u   /* Note 3                                                      */
#define ENV_REG_PAR_H1_H2_LSB   0xE2u   /* <3:0> = par_h1 low nibble, <7:4> = par_h2 low nibble        */
#define ENV_REG_PAR_H1_MSB      0xE3u
#define ENV_REG_PAR_H3          0xE4u
#define ENV_REG_PAR_H4          0xE5u
#define ENV_REG_PAR_H5          0xE6u
#define ENV_REG_PAR_H6          0xE7u
#define ENV_REG_PAR_H7          0xE8u
#define ENV_REG_PAR_T1_LSB      0xE9u
#define ENV_REG_PAR_T1_MSB      0xEAu
#define ENV_REG_PAR_G2_LSB      0xEBu
#define ENV_REG_PAR_G2_MSB      0xECu
#define ENV_REG_PAR_G1          0xEDu
#define ENV_REG_PAR_G3          0xEEu

                                        /* ---- Block 3: 0x00 ... 0x02 (page 1) ---------------------- */
#define ENV_CALIB3_START        0x00u
#define ENV_CALIB3_LEN          3u
#define ENV_REG_RES_HEAT_VAL    0x00u
#define ENV_REG_RES_HEAT_RANGE  0x02u   /* res_heat_range = bits <5:4>                                 */
#define ENV_RES_HEAT_RANGE_MASK  0x30u
#define ENV_RES_HEAT_RANGE_SHIFT 4u

                                        /* ---- Bit handling ----------------------------------------- */
#define ENV_BYTE_BITS           8u      /* Note 2                                                      */
#define ENV_NIBBLE_BITS         4u      /* Note 3                                                      */
#define ENV_NIBBLE_MASK         0x0Fu

/*
*********************************************************************************************************
*                                       CALIBRATION DATA TYPE
*
* Note(s) : (1) Names as in the datasheet. The datasheet only gives the sign of res_heat_val; the other
*               signs are chosen as in the datasheet formulas (int16_t / int8_t casts).
*********************************************************************************************************
*/

typedef struct {
    CPU_INT16U  par_t1;                 /* Temperature                                                 */
    CPU_INT16S  par_t2;
    CPU_INT08S  par_t3;

    CPU_INT16U  par_p1;                 /* Pressure                                                    */
    CPU_INT16S  par_p2;
    CPU_INT08S  par_p3;
    CPU_INT16S  par_p4;
    CPU_INT16S  par_p5;
    CPU_INT08S  par_p6;
    CPU_INT08S  par_p7;
    CPU_INT16S  par_p8;
    CPU_INT16S  par_p9;
    CPU_INT08U  par_p10;

    CPU_INT16U  par_h1;                 /* Humidity                                                    */
    CPU_INT16U  par_h2;
    CPU_INT08S  par_h3;
    CPU_INT08S  par_h4;
    CPU_INT08S  par_h5;
    CPU_INT08U  par_h6;
    CPU_INT08S  par_h7;

    CPU_INT08S  par_g1;                 /* Gas heater                                                  */
    CPU_INT16S  par_g2;
    CPU_INT08S  par_g3;
    CPU_INT08U  res_heat_range;
    CPU_INT08S  res_heat_val;
} ENV_CALIB;

/*
*********************************************************************************************************
*                               CONTROL REGISTERS (table 22, all on page 1)
*********************************************************************************************************
*/

#define ENV_REG_RES_HEAT_0      0x5Au   /* Heater resistance code, heater step 0                       */
#define ENV_REG_GAS_WAIT_0      0x64u   /* Heater-on time, heater step 0                               */
#define ENV_REG_CTRL_GAS_1      0x71u   /* run_gas, nb_conv<3:0>                                       */
#define ENV_REG_CTRL_HUM        0x72u   /* osrs_h<2:0>                                                 */
#define ENV_REG_CTRL_MEAS       0x74u   /* osrs_t<7:5>, osrs_p<4:2>, mode<1:0>                         */
#define ENV_REG_CONFIG          0x75u   /* filter<4:2>                                                 */

#define ENV_OSRS_T_SHIFT        5u
#define ENV_OSRS_P_SHIFT        2u
#define ENV_FILTER_SHIFT        2u
#define ENV_RUN_GAS             0x20u   /* run_gas = bit 5, see settings Note 2                        */
#define ENV_MODE_SLEEP          0x00u   /* mode = 00: sleep                                            */
#define ENV_MODE_FORCED         0x01u   /* mode = 01: one measurement, then back to sleep              */

/*
*********************************************************************************************************
*                                   MEASUREMENT SETTINGS (ch. 3.5)
*
* Note(s) : (1) Values of the datasheet example "Quick start - Forced mode": humidity x1, temperature x2,
*               pressure x16, heater 300 degC for 100 ms. IIR filter off (= reset value).
*           (2) The datasheet gives run_gas at bit 4 (ch. 5.3.4.7). With bit 4 the hardware
*               test showed no gas measurement, with bit 5 it works -> bit 5 is used.
*           (3) gas_wait = factor<7:6> x time<5:0> ms: 0x59 = 01 011001b = 4 x 25 ms = 100 ms.
*               Enough: the heater needs about 20-30 ms to reach its target temperature (ch. 3.6).
*           (4) The heater code depends slightly on the ambient temperature; a fixed room temperature
*               is good enough because the heater runs at 300 degC.
*********************************************************************************************************
*/

#define ENV_OSRS_H              0x01u   /* Humidity oversampling x1 (Note 1)                           */
#define ENV_OSRS_T              0x02u   /* Temperature oversampling x2                                 */
#define ENV_OSRS_P              0x05u   /* Pressure oversampling x16                                   */
#define ENV_FILTER_OFF          0x00u   /* IIR filter off                                              */
#define ENV_NB_CONV_STEP_0      0x00u   /* Forced mode uses heater step 0                              */

#define ENV_CTRL_MEAS_OSRS      ((ENV_OSRS_T << ENV_OSRS_T_SHIFT) | (ENV_OSRS_P << ENV_OSRS_P_SHIFT))
                                        /* ctrl_meas without mode bits                                 */

#define ENV_HEAT_TARGET_DEGC    300     /* Heater target temperature in degC                           */
#define ENV_HEAT_AMB_DEGC       25      /* Assumed ambient temperature in degC (Note 4)                */
#define ENV_GAS_WAIT_100MS      0x59u   /* Heater-on time 100 ms (Note 3)                              */

/*
*********************************************************************************************************
*                                   DATA REGISTERS, FIELD 0 (ch. 5.3.5)
*
* Note(s) : (1) Forced mode only uses field 0. It is read at once from 0x1D to 0x2D.
*           (2) Temperature and pressure are 20 bit: msb = bits 19:12, lsb = 11:4, xlsb<7:4> = 3:0.
*           (3) Gas is 10 bit: gas_r_msb = bits 9:2, gas_r_lsb<7:6> = bits 1:0.
*********************************************************************************************************
*/

#define ENV_FIELD0_START        0x1Du
#define ENV_FIELD0_LEN          17u
#define ENV_REG_MEAS_STATUS_0   0x1Du   /* new_data<7>                                                 */
#define ENV_REG_PRESS_MSB       0x1Fu   /* Note 2                                                      */
#define ENV_REG_PRESS_LSB       0x20u
#define ENV_REG_PRESS_XLSB      0x21u
#define ENV_REG_TEMP_MSB        0x22u   /* Note 2                                                      */
#define ENV_REG_TEMP_LSB        0x23u
#define ENV_REG_TEMP_XLSB       0x24u
#define ENV_REG_HUM_MSB         0x25u   /* 16 bit                                                      */
#define ENV_REG_HUM_LSB         0x26u
#define ENV_REG_GAS_R_MSB       0x2Cu   /* Note 3                                                      */
#define ENV_REG_GAS_R_LSB       0x2Du   /* gas_r<7:6>, gas_valid<5>, heat_stab<4>, gas_range<3:0>      */

#define ENV_NEW_DATA            0x80u   /* Measurement finished                                        */
#define ENV_GAS_VALID           0x20u   /* Gas value was measured                                      */
#define ENV_HEAT_STAB           0x10u   /* Heater reached its target temperature                       */
#define ENV_GAS_RANGE_MASK      0x0Fu

#define ENV_MSB_SHIFT_20BIT     12u     /* Note 2                                                      */
#define ENV_LSB_SHIFT_20BIT     4u
#define ENV_GAS_MSB_SHIFT       2u      /* Note 3                                                      */
#define ENV_GAS_LSB_SHIFT       6u

/*
*********************************************************************************************************
*                                         RAW DATA TYPE
*********************************************************************************************************
*/

typedef struct {
    CPU_INT32U   press_adc;             /* Raw pressure, 20 bit                                        */
    CPU_INT32U   temp_adc;              /* Raw temperature, 20 bit                                     */
    CPU_INT16U   hum_adc;               /* Raw humidity, 16 bit                                        */
    CPU_INT16U   gas_adc;               /* Raw gas resistance, 10 bit                                  */
    CPU_INT08U   gas_range;             /* Gas ADC range, 4 bit                                        */
    CPU_BOOLEAN  new_data;              /* DEF_YES: field 0 contains a new measurement                 */
    CPU_BOOLEAN  gas_valid;             /* DEF_YES: gas was measured                                   */
    CPU_BOOLEAN  heat_stab;             /* DEF_YES: heater was at its target temperature               */
} ENV_RAW;

/*
*********************************************************************************************************
*                                     COMPENSATED DATA TYPE
*
* Note(s) : (1) Fixed point integers instead of float, e.g. 2264 = 22.64 degC, 46621 = 46.621 %rH.
*********************************************************************************************************
*/

typedef struct {
    CPU_INT32S   temp_c100;             /* Temperature in 0.01 degC                                    */
    CPU_INT32U   press_pa;              /* Pressure in Pa (1 hPa = 100 Pa)                             */
    CPU_INT32U   hum_m1000;             /* Relative humidity in 0.001 %rH                              */
    CPU_INT32U   gas_ohm;               /* Gas resistance in Ohm                                       */
    CPU_BOOLEAN  gas_ok;                /* DEF_YES: gas_valid and heat_stab set -> gas_ohm usable      */
} ENV_DATA;

#define ENV_HUM_M1000_MAX       100000u /* 100.000 %rH                                                 */

/*
*********************************************************************************************************
*                                          ERROR CODES
*********************************************************************************************************
*/

typedef enum {
    ENV_ERR_NONE = 0,
    ENV_ERR_ARG,                        /* Invalid argument (null pointer, length 0, page crossing)    */
    ENV_ERR_SPI,                        /* SPI transfer failed                                         */
    ENV_ERR_CHIP_ID,                    /* Wrong chip id: no BME688 answers                            */
    ENV_ERR_VARIANT_ID                  /* Chip id fits, variant id does not                           */
} ENV_ERR;

#endif
/* [] END OF FILE */
