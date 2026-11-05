/*
*********************************************************************************************************
*                                         BSP - SPI BUS (Click Shield)
*
* Filename      : bsp_spi.c
*
* Description   : Wrapper around the PSoC SPI master SPIM_1 and the chip select pin SPI_CS.
*********************************************************************************************************
*/

#include <bsp_spi.h>

/*
*********************************************************************************************************
*                                             init_spi()
*
* Description : Starts the SPI master and puts the chip select line into its idle (inactive) state.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_PostInit().
*
* Note(s)     : (1) CS is also "initial drive state = high" in TopDesign, so the sensor is not selected
*                   between reset and this call. Set here again so the start state is defined in code too.
*********************************************************************************************************
*/

CPU_VOID init_spi(CPU_VOID)
{
    SPI_CS_Write(SPI_CS_INACTIVE);                              /* Sensor not selected (Note 1)         */
    SPIM_1_Start();                                             /* Start the SPI master                 */
}

/*
*********************************************************************************************************
*                                            spi_select()
*
* Description : Starts an SPI transaction: empties the receive FIFO and pulls CS low.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : env_raw_read(), env_raw_write().
*
* Note(s)     : (1) Otherwise old bytes would be read as the answer of the new transfer.
*               (2) CS is driven by software, not by SPIM_1, so it stays low for a whole multi-byte
*                   transfer (auto-increment read of the BME688).
*********************************************************************************************************
*/

CPU_VOID spi_select(CPU_VOID)
{
    SPIM_1_ClearRxBuffer();                                     /* Discard stale bytes (Note 1)         */
    SPI_CS_Write(SPI_CS_ACTIVE);                                /* Begin of transaction (Note 2)        */
}

/*
*********************************************************************************************************
*                                           spi_deselect()
*
* Description : Ends an SPI transaction by releasing CS.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : env_raw_read(), env_raw_write().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_VOID spi_deselect(CPU_VOID)
{
    SPI_CS_Write(SPI_CS_INACTIVE);                              /* End of transaction                   */
}

/*
*********************************************************************************************************
*                                           spi_transfer()
*
* Description : Sends one byte and returns the byte received at the same time.
*
* Argument(s) : tx_byte     byte to send (SPI_DUMMY_BYTE when only reading).
*
*               p_rx_byte   pointer to the variable that receives the incoming byte.
*
* Return(s)   : DEF_OK      byte transferred.
*               DEF_FAIL    invalid pointer or no byte received within SPI_BYTE_WAIT_LOOPS_MAX.
*
* Caller(s)   : env_raw_read(), env_raw_write().
*
* Note(s)     : (1) SPI is full duplex: every byte sent clocks in one byte. To read, a dummy byte is sent.
*               (2) Deliberate exception to "no blocking in the BSP": one byte takes a few microseconds,
*                   far less than one OS tick (1 ms), so an OS delay is not possible. The wait is
*                   limited by SPI_BYTE_WAIT_LOOPS_MAX, so it cannot hang.
*               (3) Not re-entrant: a second SPI user would need a mutex.
*********************************************************************************************************
*/

CPU_BOOLEAN spi_transfer(CPU_INT08U tx_byte, CPU_INT08U *p_rx_byte)
{
    CPU_INT32U wait_cnt;


    if (p_rx_byte == (CPU_INT08U *)0) {                         /* Check argument                       */
        return (DEF_FAIL);
    }

    SPIM_1_WriteTxData(tx_byte);                                /* Send byte (Note 1)                   */

    wait_cnt = 0u;                                              /* Wait for received byte (Note 2)      */
    while ((SPIM_1_GetRxBufferSize() == 0u) &&
           (wait_cnt < SPI_BYTE_WAIT_LOOPS_MAX)) {
        wait_cnt++;
    }

    if (SPIM_1_GetRxBufferSize() == 0u) {                       /* Nothing received: hardware problem   */
        return (DEF_FAIL);
    }

    *p_rx_byte = SPIM_1_ReadRxData();                           /* Return received byte                 */
    return (DEF_OK);
}

/* [] END OF FILE */
