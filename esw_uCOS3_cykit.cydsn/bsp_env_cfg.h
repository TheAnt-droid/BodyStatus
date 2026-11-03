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