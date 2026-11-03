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
