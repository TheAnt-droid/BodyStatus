/*
*********************************************************************************************************
*                                    ENVIRONMENT 3 CLICK (BME688) - DRIVER
*
* Filename      : bsp_env.c
*
* Description   : Register access to the BME688 via SPI (mikroBUS slot 1 of the Click Shield).
*                 Source: Bosch Sensortec, BME688 Datasheet, BST-BME688-DS000-00 Rev. 1.0 (04/2021).
*
* Note(s)       : (1) Not re-entrant: only one task may use this driver (and the SPI bus). If a second
*                     task needs SPI later, protect the bus with a mutex.
*********************************************************************************************************
*/

#include <bsp_env.h>

/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static ENV_ERR env_raw_read     (CPU_INT08U reg, CPU_INT08U *p_buf, CPU_INT08U len);
static ENV_ERR env_raw_write    (CPU_INT08U reg, CPU_INT08U val);
static ENV_ERR env_select_page  (CPU_INT08U reg);

/*
*********************************************************************************************************
*                                          env_read_regs()
*
* Description : Reads one or more consecutive registers of the BME688.
*
* Argument(s) : reg     full 8-bit register address (see bsp_env_cfg.h, Note 1).
*
*               p_buf   buffer for the read bytes.
*
*               len     number of registers to read (at least 1).
*
* Return(s)   : ENV_ERR_NONE    registers read.
*               ENV_ERR_ARG     invalid argument (see Note 2).
*               ENV_ERR_SPI     SPI transfer failed.
*
* Caller(s)   : Application, env_check_id().
*
* Note(s)     : (1) The sensor increments the address itself, so several registers are read with one
*                   control byte (datasheet ch. 6.3, picture 11).
*               (2) All registers of one read must be on the same memory page, because the page cannot
*                   be switched in the middle of a transaction.
*********************************************************************************************************
*/

ENV_ERR env_read_regs(CPU_INT08U reg, CPU_INT08U *p_buf, CPU_INT08U len)
{
    CPU_INT16U  last_reg;
    ENV_ERR     err;


    if ((p_buf == (CPU_INT08U *)0) || (len == 0u)) {            /* Check arguments                      */
        return (ENV_ERR_ARG);
    }

    last_reg = (CPU_INT16U)((CPU_INT16U)reg + len - 1u);        /* Must stay on the same page (Note 2)  */
    if ((last_reg > DEF_INT_08U_MAX_VAL) ||
        ((reg & ENV_REG_PAGE_SEL_BIT) != ((CPU_INT08U)last_reg & ENV_REG_PAGE_SEL_BIT))) {
        return (ENV_ERR_ARG);
    }

    err = env_select_page(reg);                                 /* Switch to the page of the register   */
    if (err != ENV_ERR_NONE) {
        return (err);
    }

    return (env_raw_read(reg, p_buf, len));                     /* Read registers (Note 1)              */
}

/*
*********************************************************************************************************
*                                          env_write_reg()
*
* Description : Writes one register of the BME688.
*
* Argument(s) : reg     full 8-bit register address (see bsp_env_cfg.h, Note 1).
*
*               val     value to write.
*
* Return(s)   : ENV_ERR_NONE    register written.
*               ENV_ERR_SPI     SPI transfer failed.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

ENV_ERR env_write_reg(CPU_INT08U reg, CPU_INT08U val)
{
    ENV_ERR err;


    err = env_select_page(reg);                                 /* Switch to the page of the register   */
    if (err != ENV_ERR_NONE) {
        return (err);
    }

    return (env_raw_write(reg, val));                           /* Write register                       */
}

/*
*********************************************************************************************************
*                                           env_check_id()
*
* Description : Checks whether a BME688 answers on the SPI bus by reading chip id and variant id.
*
* Argument(s) : none.
*
* Return(s)   : ENV_ERR_NONE        BME688 found.
*               ENV_ERR_SPI         SPI transfer failed.
*               ENV_ERR_CHIP_ID     wrong chip id (Note 1).
*               ENV_ERR_VARIANT_ID  chip id correct, but no BME688 variant (Note 2).
*
* Caller(s)   : Application (once at start, before configuring the sensor).
*
* Note(s)     : (1) If MISO does not reach the PSoC (switch S2 "SPI Bridge" = OFF, COMM SEL jumpers on
*                   I2C, wrong pins), typically 0x00 or 0xFF is read instead of 0x61.
*               (2) The BME680 has the same chip id. The variant id 0x01 identifies the BME688
*                   (datasheet ch. 5.3.2.1).
*********************************************************************************************************
*/

ENV_ERR env_check_id(CPU_VOID)
{
    CPU_INT08U  id;
    ENV_ERR     err;


    err = env_read_regs(ENV_REG_CHIP_ID, &id, 1u);              /* Read chip id                         */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    if (id != ENV_CHIP_ID_VAL) {                                /* Expected 0x61 (Note 1)               */
        return (ENV_ERR_CHIP_ID);
    }

    err = env_read_regs(ENV_REG_VARIANT_ID, &id, 1u);           /* Read variant id                      */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    if (id != ENV_VARIANT_ID_VAL) {                             /* Expected 0x01 (Note 2)               */
        return (ENV_ERR_VARIANT_ID);
    }

    return (ENV_ERR_NONE);
}

/*
*********************************************************************************************************
*                                          env_raw_read()
*
* Description : Reads registers on the currently active page (no page switch).
*
* Argument(s) : reg     full 8-bit register address, only the lower 7 bits are sent.
*
*               p_buf   buffer for the read bytes.
*
*               len     number of registers to read.
*
* Return(s)   : ENV_ERR_NONE or ENV_ERR_SPI.
*
* Caller(s)   : env_read_regs(), env_select_page().
*
* Note(s)     : (1) Control byte = 7-bit address with bit 7 = 1 (read), datasheet ch. 6.3. The byte
*                   received while sending it has no meaning.
*               (2) Each dummy byte clocks one register value in.
*********************************************************************************************************
*/

static ENV_ERR env_raw_read(CPU_INT08U reg, CPU_INT08U *p_buf, CPU_INT08U len)
{
    CPU_INT08U  i;
    CPU_INT08U  ignored;
    CPU_BOOLEAN ok;


    spi_select();                                               /* CS low: start of transaction         */

    ok = spi_transfer((CPU_INT08U)((reg & ENV_SPI_ADDR_MASK) | ENV_SPI_RW_READ),
                      &ignored);                                /* Send control byte (Note 1)           */

    for (i = 0u; (i < len) && (ok == DEF_OK); i++) {            /* Read data bytes (Note 2)             */
        ok = spi_transfer(SPI_DUMMY_BYTE, &p_buf[i]);
    }

    spi_deselect();                                             /* CS high: end of transaction          */

    if (ok != DEF_OK) {
        return (ENV_ERR_SPI);
    }
    return (ENV_ERR_NONE);
}

/*
*********************************************************************************************************
*                                          env_raw_write()
*
* Description : Writes one register on the currently active page (no page switch).
*
* Argument(s) : reg     full 8-bit register address, only the lower 7 bits are sent.
*
*               val     value to write.
*
* Return(s)   : ENV_ERR_NONE or ENV_ERR_SPI.
*
* Caller(s)   : env_write_reg(), env_select_page().
*
* Note(s)     : (1) Control byte = 7-bit address with bit 7 = 0 (write), datasheet ch. 6.3, picture 10.
*                   The masking clears bit 7, so no extra write bit is needed.
*********************************************************************************************************
*/

static ENV_ERR env_raw_write(CPU_INT08U reg, CPU_INT08U val)
{
    CPU_INT08U  ignored;
    CPU_BOOLEAN ok;


    spi_select();                                               /* CS low: start of transaction         */

    ok = spi_transfer((CPU_INT08U)(reg & ENV_SPI_ADDR_MASK),    /* Send control byte (Note 1)           */
                      &ignored);
    if (ok == DEF_OK) {
        ok = spi_transfer(val, &ignored);                       /* Send data byte                       */
    }

    spi_deselect();                                             /* CS high: end of transaction          */

    if (ok != DEF_OK) {
        return (ENV_ERR_SPI);
    }
    return (ENV_ERR_NONE);
}

/*
*********************************************************************************************************
*                                         env_select_page()
*
* Description : Activates the SPI memory page on which the given register lies.
*
* Argument(s) : reg     full 8-bit register address of the register that is accessed next.
*
* Return(s)   : ENV_ERR_NONE or ENV_ERR_SPI.
*
* Caller(s)   : env_read_regs(), env_write_reg().
*
* Note(s)     : (1) Full address 0x80..0xFF -> page 0 (spi_mem_page = 0),
*                   full address 0x00..0x7F -> page 1 (spi_mem_page = 1), table 21.
*               (2) The status register is reachable from both pages (ch. 5.3.1.4), so it can be
*                   accessed without switching first.
*               (3) The current page is read from the sensor instead of being stored in a variable.
*                   This costs one short SPI read, but needs no global variable and stays correct
*                   even after a sensor reset.
*               (4) Read-modify-write, so that the other bits of the status register are kept.
*********************************************************************************************************
*/

static ENV_ERR env_select_page(CPU_INT08U reg)
{
    CPU_INT08U  status;
    CPU_INT08U  status_new;
    ENV_ERR     err;


    err = env_raw_read(ENV_REG_STATUS, &status, 1u);            /* Read current page (Notes 2, 3)       */
    if (err != ENV_ERR_NONE) {
        return (err);
    }

    if ((reg & ENV_REG_PAGE_SEL_BIT) != 0u) {                   /* Register on page 0 (Note 1)          */
        status_new = (CPU_INT08U)(status & (CPU_INT08U)~ENV_STATUS_MEM_PAGE);
    } else {                                                    /* Register on page 1 (Note 1)          */
        status_new = (CPU_INT08U)(status | ENV_STATUS_MEM_PAGE);
    }

    if (status_new == status) {                                 /* Already on the right page            */
        return (ENV_ERR_NONE);
    }

    return (env_raw_write(ENV_REG_STATUS, status_new));         /* Switch page (Note 4)                 */
}

/* [] END OF FILE */
