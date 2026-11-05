/*
*********************************************************************************************************
*                                         BSP - SPI BUS (Click Shield)
*
* Filename      : bsp_spi.h
*
* Description   : Wrapper around the PSoC SPI master SPIM_1 and the chip select pin SPI_CS.
*                 Only this file talks to the generated SPI APIs - sensor drivers (bsp_env.c)
*                 use the functions below.
*
* Pins (Click Shield, SPI of mikroBUS slot 1):
*                 MOSI P2_3, MISO P12_2 (via switch S2 "SPI Bridge" = ON), SCLK P12_3, CS P12_4
*********************************************************************************************************
*/

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <cpu.h>
#include <lib_def.h>
#include <SPIM_1.h>
#include <SPI_CS.h>

/*
*********************************************************************************************************
*                                              DEFINES
*********************************************************************************************************
*/

#define SPI_CS_ACTIVE           0u      /* Chip select is active low (BME688 datasheet ch. 6.3)        */
#define SPI_CS_INACTIVE         1u      /* Idle level of CS (also initial state in TopDesign)          */

#define SPI_DUMMY_BYTE          0x00u   /* Sent while reading (SPI sends and receives at once)         */

#define SPI_BYTE_WAIT_LOOPS_MAX 1000u   /* Max. wait loops per byte; only reached if the SPI hardware  */
                                        /* hangs (one byte takes 8 us at 1 Mbit/s)                     */

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_VOID    init_spi        (CPU_VOID);
CPU_VOID    spi_select      (CPU_VOID);
CPU_VOID    spi_deselect    (CPU_VOID);
CPU_BOOLEAN spi_transfer    (CPU_INT08U tx_byte, CPU_INT08U *p_rx_byte);

#endif
/* [] END OF FILE */
