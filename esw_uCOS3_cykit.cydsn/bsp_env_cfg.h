/*
*********************************************************************************************************
*                                  ENVIRONMENT 3 CLICK (BME688) - CONFIGURATION
*
* Filename      : bsp_env_cfg.h
*
* Description   : Register addresses, register values and types of the BME688.
*                 Source: Bosch Sensortec, BME688 Datasheet, BST-BME688-DS000-00 Rev. 1.0 (04/2021).
*
* Note(s)       : (1) All register addresses are the full 8-bit addresses (column "I2C Adr" of
*                     table 22). In SPI mode the sensor only sees 7 address bits; bit 7 of the full
*                     address tells which memory page the register is on (table 21):
*                         full address 0x80..0xFF -> SPI page 0
*                         full address 0x00..0x7F -> SPI page 1
*                     bsp_env.c derives page and 7-bit SPI address from the full address.
*                 (2) Datasheet ch. 5.3.1.4 describes the pages the other way round in the text.
*                     Tables 21 and 22 are consistent with each other, so the tables are used.
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

#define ENV_SPI_RW_READ         0x80u   /* Bit 7 of the control byte: 1 = read                          */
#define ENV_SPI_ADDR_MASK       0x7Fu   /* Only 7 address bits are transferred via SPI                 */
#define ENV_REG_PAGE_SEL_BIT    0x80u   /* Bit 7 of the full address: 1 -> page 0, 0 -> page 1 (Note 1) */

/*
*********************************************************************************************************
*                                    REGISTER ADDRESSES (table 22)
*********************************************************************************************************
*/

#define ENV_REG_STATUS          0x73u   /* Contains spi_mem_page, reachable from both pages (5.3.1.4)  */
#define ENV_REG_CHIP_ID         0xD0u   /* Chip id, SPI: 0x50 on page 0                                */
#define ENV_REG_VARIANT_ID      0xF0u   /* Variant id, SPI: 0x70 on page 0                             */

/*
*********************************************************************************************************
*                                         REGISTER VALUES
*********************************************************************************************************
*/

#define ENV_STATUS_MEM_PAGE     0x10u   /* status<4> = spi_mem_page: 0 = page 0, 1 = page 1 (5.3.1.4)  */

#define ENV_CHIP_ID_VAL         0x61u   /* Reset state of Chip_Id (table 22)                           */
#define ENV_VARIANT_ID_VAL      0x01u   /* Variant id of the BME688 (5.3.2.1)                          */

/*
*********************************************************************************************************
*                          CALIBRATION REGISTERS (tables 13, 14, 15, 16, ch. 3.6)
*
* Note(s) : (1) The calibration values are spread over three register blocks. Each block is read with
*               one multi-byte read (auto-increment). Blocks 1 and 2 are on SPI page 0, block 3 on page 1.
*           (2) "_LSB" / "_MSB" as in the tables: value = (MSB << 8) | LSB.
*           (3) par_h1 and par_h2 are 12 bit values: the 4 low bits share register 0xE2
*               (par_h1 = 0xE2<3:0>, par_h2 = 0xE2<7:4>), the 8 high bits are in 0xE3 / 0xE1.
*********************************************************************************************************
*/

                                        /* ---------------- Block 1: 0x8A ... 0xA0 (page 0) ----------- */
#define ENV_CALIB1_START        0x8Au   /* First register of block 1                                   */
#define ENV_CALIB1_LEN          23u     /* 0x8A ... 0xA0                                               */
#define ENV_REG_PAR_T2_LSB      0x8Au   /* Table 13                                                    */
#define ENV_REG_PAR_T2_MSB      0x8Bu
#define ENV_REG_PAR_T3          0x8Cu
#define ENV_REG_PAR_P1_LSB      0x8Eu   /* Table 14                                                    */
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

                                        /* ---------------- Block 2: 0xE1 ... 0xEE (page 0) ----------- */
#define ENV_CALIB2_START        0xE1u   /* First register of block 2                                   */
#define ENV_CALIB2_LEN          14u     /* 0xE1 ... 0xEE                                               */
#define ENV_REG_PAR_H2_MSB      0xE1u   /* Table 15 (Note 3)                                           */
#define ENV_REG_PAR_H1_H2_LSB   0xE2u   /* <3:0> = par_h1 low nibble, <7:4> = par_h2 low nibble        */
#define ENV_REG_PAR_H1_MSB      0xE3u
#define ENV_REG_PAR_H3          0xE4u
#define ENV_REG_PAR_H4          0xE5u
#define ENV_REG_PAR_H5          0xE6u
#define ENV_REG_PAR_H6          0xE7u
#define ENV_REG_PAR_H7          0xE8u
#define ENV_REG_PAR_T1_LSB      0xE9u   /* Table 13                                                    */
#define ENV_REG_PAR_T1_MSB      0xEAu
#define ENV_REG_PAR_G2_LSB      0xEBu   /* Table 16                                                    */
#define ENV_REG_PAR_G2_MSB      0xECu
#define ENV_REG_PAR_G1          0xEDu
#define ENV_REG_PAR_G3          0xEEu

                                        /* ---------------- Block 3: 0x00 ... 0x02 (page 1) ----------- */
#define ENV_CALIB3_START        0x00u   /* First register of block 3                                   */
#define ENV_CALIB3_LEN          3u      /* 0x00 ... 0x02                                               */
#define ENV_REG_RES_HEAT_VAL    0x00u   /* Table 16: signed, -128 ... 127                              */
#define ENV_REG_RES_HEAT_RANGE  0x02u   /* Table 16: res_heat_range = 0x02<5:4>                        */

#define ENV_RES_HEAT_RANGE_MASK  0x30u  /* Bits <5:4> of register 0x02                                 */
#define ENV_RES_HEAT_RANGE_SHIFT 4u

                                        /* ---------------- Bit handling ----------------------------- */
#define ENV_BYTE_BITS           8u      /* MSB is shifted by 8 bit (Note 2)                            */
#define ENV_NIBBLE_BITS         4u      /* 12 bit values: MSB is shifted by 4 bit (Note 3)             */
#define ENV_NIBBLE_MASK         0x0Fu   /* Lower 4 bit                                                 */

/*
*********************************************************************************************************
*                                       CALIBRATION DATA TYPE
*
* Note(s) : (1) Names as in the datasheet (tables 13 to 16).
*           (2) The datasheet only states the sign of res_heat_val (signed). For the other parameters
*               it gives no sign; the types below are an assumption that is checked with the
*               plausibility of the compensated values (step 7).
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

    CPU_INT16U  par_h1;                 /* Humidity (12 bit values, see calibration registers Note 3)  */
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
*                          MEASUREMENT CONTROL REGISTERS (table 22, ch. 5.3.1, 5.3.3, 5.3.4)
*
* Note(s) : (1) All control and data registers are on SPI page 1 (full address 0x00 ... 0x7F).
*********************************************************************************************************
*/

#define ENV_REG_RES_HEAT_0      0x5Au   /* Target heater resistance of heater step 0 (5.3.4.2)         */
#define ENV_REG_GAS_WAIT_0      0x64u   /* Heater-on time of heater step 0 (5.3.4.3)                   */
#define ENV_REG_CTRL_GAS_1      0x71u   /* run_gas, nb_conv<3:0> (5.3.4.6, 5.3.4.7, see settings Note 2)*/
#define ENV_REG_CTRL_HUM        0x72u   /* osrs_h<2:0> (5.3.3.1)                                       */
#define ENV_REG_CTRL_MEAS       0x74u   /* osrs_t<7:5>, osrs_p<4:2>, mode<1:0> (5.3.3.2, 5.3.3.3)      */
#define ENV_REG_CONFIG          0x75u   /* filter<4:2>, spi_3w_en<0> (5.3.3.4)                         */

                                        /* ---------------- Bit fields ------------------------------- */
#define ENV_OSRS_T_SHIFT        5u      /* ctrl_meas: osrs_t<7:5>                                      */
#define ENV_OSRS_P_SHIFT        2u      /* ctrl_meas: osrs_p<4:2>                                      */
#define ENV_FILTER_SHIFT        2u      /* config:    filter<4:2>                                      */
#define ENV_RUN_GAS             0x20u   /* ctrl_gas_1: run_gas = bit 5 (Note 2 of settings)            */
#define ENV_MODE_SLEEP          0x00u   /* mode<1:0> = 00 (table 12)                                   */
#define ENV_MODE_FORCED         0x01u   /* mode<1:0> = 01: one measurement, then back to sleep         */

/*
*********************************************************************************************************
*                                   MEASUREMENT SETTINGS (ch. 3.5)
*
* Note(s) : (1) The settings are the example of the datasheet, "Quick start - Forced mode" (ch. 3.5):
*               humidity x1, temperature x2, pressure x16, heater 300 degC for 100 ms, IIR filter off.
*           (2) Datasheet ch. 5.3.4.7 and table 22 give run_gas at bit 4, and the quick start calls the
*               bit "run_gas_l" - the name used for the predecessor BME680. With bit 4 the hardware test
*               showed no gas measurement (gas_valid_r = 0, heat_stab_r = 0). With bit 5 (0x20) the gas
*               measurement runs (gas_valid_r = 1, heat_stab_r = 1, test 03.11.2026). Bit 5 is used.
*           (3) gas_wait<7:6> = multiplication factor (01 = x4), gas_wait<5:0> = time in 1 ms steps
*               (5.3.4.3): 0x59 = 01 011001b = 4 x 25 ms = 100 ms (value from the quick start example).
*           (4) The heater resistance depends on the ambient temperature (ch. 3.6). A fixed room
*               temperature is used here; the error is small because the heater runs at 300 degC.
*********************************************************************************************************
*/

#define ENV_OSRS_H              0x01u   /* Humidity oversampling x1    (5.3.3.1, Note 1)               */
#define ENV_OSRS_T              0x02u   /* Temperature oversampling x2 (5.3.3.2, Note 1)               */
#define ENV_OSRS_P              0x05u   /* Pressure oversampling x16   (5.3.3.3, Note 1)               */
#define ENV_FILTER_OFF          0x00u   /* IIR filter coefficient 0    (5.3.3.4)                       */
#define ENV_NB_CONV_STEP_0      0x00u   /* Forced mode uses heater step 0 (5.3.4.6)                    */

#define ENV_HEAT_TARGET_DEGC    300     /* Heater target temperature in degC (Note 1)                  */
#define ENV_HEAT_AMB_DEGC       25      /* Assumed ambient temperature in degC (Note 4)                */
#define ENV_GAS_WAIT_100MS      0x59u   /* Heater-on time 100 ms (Note 3)                              */

/*
*********************************************************************************************************
*                                  DATA REGISTERS, FIELD 0 (ch. 5.3.5, 5.3.6)
*
* Note(s) : (1) In forced mode only field 0 is used (ch. 5.3.5). Field 0 is read with one multi-byte read
*               from meas_status_0 (0x1D) up to gas_r_lsb (0x2D).
*           (2) Temperature and pressure are 20 bit: msb = bits 19:12, lsb = bits 11:4, xlsb<7:4> = 3:0.
*           (3) Gas ADC value is 10 bit: gas_r_msb = bits 9:2, gas_r_lsb<7:6> = bits 1:0.
*********************************************************************************************************
*/

#define ENV_FIELD0_START        0x1Du   /* meas_status_0                                               */
#define ENV_FIELD0_LEN          17u     /* 0x1D ... 0x2D                                               */
#define ENV_REG_MEAS_STATUS_0   0x1Du   /* new_data<7>, gas_measuring<6>, measuring<5> (5.3.6)          */
#define ENV_REG_PRESS_MSB       0x1Fu   /* Note 2                                                      */
#define ENV_REG_PRESS_LSB       0x20u
#define ENV_REG_PRESS_XLSB      0x21u
#define ENV_REG_TEMP_MSB        0x22u   /* Note 2                                                      */
#define ENV_REG_TEMP_LSB        0x23u
#define ENV_REG_TEMP_XLSB       0x24u
#define ENV_REG_HUM_MSB         0x25u   /* 16 bit: msb = bits 15:8, lsb = bits 7:0 (5.3.5.3)           */
#define ENV_REG_HUM_LSB         0x26u
#define ENV_REG_GAS_R_MSB       0x2Cu   /* Note 3                                                      */
#define ENV_REG_GAS_R_LSB       0x2Du   /* gas_r<7:6>, gas_valid_r<5>, heat_stab_r<4>, gas_range_r<3:0> */

#define ENV_NEW_DATA            0x80u   /* meas_status: new_data<7> (5.3.6.1)                          */
#define ENV_GAS_VALID           0x20u   /* gas_r_lsb:   gas_valid_r<5> (5.3.6.5)                       */
#define ENV_HEAT_STAB           0x10u   /* gas_r_lsb:   heat_stab_r<4> (5.3.6.6)                       */
#define ENV_GAS_RANGE_MASK      0x0Fu   /* gas_r_lsb:   gas_range_r<3:0> (5.3.5.5)                     */

#define ENV_MSB_SHIFT_20BIT     12u     /* Note 2: msb -> bits 19:12                                   */
#define ENV_LSB_SHIFT_20BIT     4u      /* Note 2: lsb -> bits 11:4, xlsb<7:4> -> bits 3:0             */
#define ENV_GAS_MSB_SHIFT       2u      /* Note 3: gas_r_msb -> bits 9:2                               */
#define ENV_GAS_LSB_SHIFT       6u      /* Note 3: gas_r_lsb<7:6> -> bits 1:0                          */

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
    CPU_BOOLEAN  gas_valid;             /* DEF_YES: real gas measurement                               */
    CPU_BOOLEAN  heat_stab;             /* DEF_YES: heater reached its target temperature              */
} ENV_RAW;

/*
*********************************************************************************************************
*                                          ERROR CODES
*********************************************************************************************************
*/

typedef enum {
    ENV_ERR_NONE = 0,                   /* No error                                                    */
    ENV_ERR_ARG,                        /* Invalid argument (null pointer, length 0, page crossing)    */
    ENV_ERR_SPI,                        /* SPI transfer failed (no byte received)                      */
    ENV_ERR_CHIP_ID,                    /* Wrong chip id: no BME688 answers (wiring, S2, jumpers?)     */
    ENV_ERR_VARIANT_ID                  /* Chip id fits but variant id does not (e.g. a BME680)        */
} ENV_ERR;

#endif
/* [] END OF FILE */