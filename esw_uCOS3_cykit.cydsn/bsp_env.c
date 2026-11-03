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
#include <bsp_env_calc.h>

/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static ENV_ERR env_raw_read     (CPU_INT08U reg, CPU_INT08U *p_buf, CPU_INT08U len);
static ENV_ERR env_raw_write    (CPU_INT08U reg, CPU_INT08U val);
static ENV_ERR env_select_page  (CPU_INT08U reg);
static CPU_INT16U env_u16       (CPU_INT08U msb, CPU_INT08U lsb);
static CPU_INT32U env_u20       (CPU_INT08U msb, CPU_INT08U lsb, CPU_INT08U xlsb);

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
*                                          env_read_calib()
*
* Description : Reads the factory calibration values of the BME688 into a structure.
*
* Argument(s) : p_calib     structure that receives the calibration values.
*
* Return(s)   : ENV_ERR_NONE    calibration values read.
*               ENV_ERR_ARG     p_calib is a null pointer.
*               ENV_ERR_SPI     SPI transfer failed.
*
* Caller(s)   : Application (once at start, the values never change).
*
* Note(s)     : (1) The values are spread over three register blocks (bsp_env_cfg.h). Each block is
*                   read with one multi-byte read; env_read_regs() switches the SPI page if needed.
*               (2) The index into a block buffer is "register address - first address of the block",
*                   so every value can be compared 1:1 with the address in tables 13 to 16.
*               (3) The casts to a signed type re-interpret the bits read from the sensor (two's
*                   complement), see the Note on the data types in bsp_env_cfg.h.
*               (4) The buffers are local variables: the function is re-entrant as long as the SPI bus
*                   itself is only used by one task.
*********************************************************************************************************
*/

ENV_ERR env_read_calib(ENV_CALIB *p_calib)
{
    CPU_INT08U  blk1[ENV_CALIB1_LEN];
    CPU_INT08U  blk2[ENV_CALIB2_LEN];
    CPU_INT08U  blk3[ENV_CALIB3_LEN];
    ENV_ERR     err;


    if (p_calib == (ENV_CALIB *)0) {                            /* Check argument                       */
        return (ENV_ERR_ARG);
    }

    err = env_read_regs(ENV_CALIB1_START, &blk1[0], ENV_CALIB1_LEN);    /* Block 1, page 0 (Note 1)    */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    err = env_read_regs(ENV_CALIB2_START, &blk2[0], ENV_CALIB2_LEN);    /* Block 2, page 0             */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    err = env_read_regs(ENV_CALIB3_START, &blk3[0], ENV_CALIB3_LEN);    /* Block 3, page 1             */
    if (err != ENV_ERR_NONE) {
        return (err);
    }

                                                                /* Temperature (table 13, Notes 2, 3)   */
    p_calib->par_t1 =              env_u16(blk2[ENV_REG_PAR_T1_MSB - ENV_CALIB2_START],
                                           blk2[ENV_REG_PAR_T1_LSB - ENV_CALIB2_START]);
    p_calib->par_t2 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_T2_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_T2_LSB - ENV_CALIB1_START]);
    p_calib->par_t3 = (CPU_INT08S) blk1[ENV_REG_PAR_T3 - ENV_CALIB1_START];

                                                                /* Pressure (table 14)                  */
    p_calib->par_p1 =              env_u16(blk1[ENV_REG_PAR_P1_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P1_LSB - ENV_CALIB1_START]);
    p_calib->par_p2 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_P2_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P2_LSB - ENV_CALIB1_START]);
    p_calib->par_p3 = (CPU_INT08S) blk1[ENV_REG_PAR_P3 - ENV_CALIB1_START];
    p_calib->par_p4 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_P4_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P4_LSB - ENV_CALIB1_START]);
    p_calib->par_p5 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_P5_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P5_LSB - ENV_CALIB1_START]);
    p_calib->par_p6 = (CPU_INT08S) blk1[ENV_REG_PAR_P6 - ENV_CALIB1_START];
    p_calib->par_p7 = (CPU_INT08S) blk1[ENV_REG_PAR_P7 - ENV_CALIB1_START];
    p_calib->par_p8 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_P8_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P8_LSB - ENV_CALIB1_START]);
    p_calib->par_p9 = (CPU_INT16S) env_u16(blk1[ENV_REG_PAR_P9_MSB - ENV_CALIB1_START],
                                           blk1[ENV_REG_PAR_P9_LSB - ENV_CALIB1_START]);
    p_calib->par_p10 =             blk1[ENV_REG_PAR_P10 - ENV_CALIB1_START];

                                                                /* Humidity (table 15), 12 bit values   */
    p_calib->par_h1 = (CPU_INT16U)(((CPU_INT16U)blk2[ENV_REG_PAR_H1_MSB - ENV_CALIB2_START] << ENV_NIBBLE_BITS) |
                                   (blk2[ENV_REG_PAR_H1_H2_LSB - ENV_CALIB2_START] & ENV_NIBBLE_MASK));
    p_calib->par_h2 = (CPU_INT16U)(((CPU_INT16U)blk2[ENV_REG_PAR_H2_MSB - ENV_CALIB2_START] << ENV_NIBBLE_BITS) |
                                   (blk2[ENV_REG_PAR_H1_H2_LSB - ENV_CALIB2_START] >> ENV_NIBBLE_BITS));
    p_calib->par_h3 = (CPU_INT08S) blk2[ENV_REG_PAR_H3 - ENV_CALIB2_START];
    p_calib->par_h4 = (CPU_INT08S) blk2[ENV_REG_PAR_H4 - ENV_CALIB2_START];
    p_calib->par_h5 = (CPU_INT08S) blk2[ENV_REG_PAR_H5 - ENV_CALIB2_START];
    p_calib->par_h6 =              blk2[ENV_REG_PAR_H6 - ENV_CALIB2_START];
    p_calib->par_h7 = (CPU_INT08S) blk2[ENV_REG_PAR_H7 - ENV_CALIB2_START];

                                                                /* Gas heater (table 16)                */
    p_calib->par_g1 = (CPU_INT08S) blk2[ENV_REG_PAR_G1 - ENV_CALIB2_START];
    p_calib->par_g2 = (CPU_INT16S) env_u16(blk2[ENV_REG_PAR_G2_MSB - ENV_CALIB2_START],
                                           blk2[ENV_REG_PAR_G2_LSB - ENV_CALIB2_START]);
    p_calib->par_g3 = (CPU_INT08S) blk2[ENV_REG_PAR_G3 - ENV_CALIB2_START];
    p_calib->res_heat_range = (CPU_INT08U)((blk3[ENV_REG_RES_HEAT_RANGE - ENV_CALIB3_START] &
                                            ENV_RES_HEAT_RANGE_MASK) >> ENV_RES_HEAT_RANGE_SHIFT);
    p_calib->res_heat_val   = (CPU_INT08S) blk3[ENV_REG_RES_HEAT_VAL - ENV_CALIB3_START];

    return (ENV_ERR_NONE);
}

/*
*********************************************************************************************************
*                                            env_config()
*
* Description : Writes the measurement settings (oversampling, filter, gas heater step 0) into the
*               BME688. The sensor stays in sleep mode; a measurement is started with env_start_meas().
*
* Argument(s) : p_calib     calibration values (needed to calculate the heater resistance code).
*
* Return(s)   : ENV_ERR_NONE    settings written.
*               ENV_ERR_ARG     p_calib is a null pointer.
*               ENV_ERR_SPI     SPI transfer failed.
*
* Caller(s)   : Application (once after env_read_calib()).
*
* Note(s)     : (1) Order as in the datasheet (ch. 3.5, figure 3): oversampling first, osrs_h before
*                   osrs_t / osrs_p, then the gas heater, the mode register last.
*               (2) Settings see "MEASUREMENT SETTINGS" in bsp_env_cfg.h.
*               (3) All registers are on SPI page 1, so the page is switched only once.
*********************************************************************************************************
*/

ENV_ERR env_config(const ENV_CALIB *p_calib)
{
    CPU_INT08U  res_heat;
    ENV_ERR     err;


    if (p_calib == (const ENV_CALIB *)0) {                      /* Check argument                       */
        return (ENV_ERR_ARG);
    }

    err = env_write_reg(ENV_REG_CTRL_HUM, ENV_OSRS_H);          /* Humidity oversampling (Note 1)       */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    err = env_write_reg(ENV_REG_CTRL_MEAS,                      /* Temp. + pressure oversampling, sleep */
                        (CPU_INT08U)((ENV_OSRS_T << ENV_OSRS_T_SHIFT) |
                                     (ENV_OSRS_P << ENV_OSRS_P_SHIFT) |
                                      ENV_MODE_SLEEP));
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    err = env_write_reg(ENV_REG_CONFIG,                         /* IIR filter off, 4-wire SPI           */
                        (CPU_INT08U)(ENV_FILTER_OFF << ENV_FILTER_SHIFT));
    if (err != ENV_ERR_NONE) {
        return (err);
    }

    res_heat = env_calc_res_heat(p_calib,                       /* Heater: 300 degC -> register code    */
                                 ENV_HEAT_TARGET_DEGC,
                                 ENV_HEAT_AMB_DEGC);
    err = env_write_reg(ENV_REG_RES_HEAT_0, res_heat);
    if (err != ENV_ERR_NONE) {
        return (err);
    }
    err = env_write_reg(ENV_REG_GAS_WAIT_0, ENV_GAS_WAIT_100MS); /* Heater-on time 100 ms               */
    if (err != ENV_ERR_NONE) {
        return (err);
    }

    return (env_write_reg(ENV_REG_CTRL_GAS_1,                   /* Enable gas, use heater step 0        */
                          (CPU_INT08U)(ENV_RUN_GAS | ENV_NB_CONV_STEP_0)));
}

/*
*********************************************************************************************************
*                                          env_start_meas()
*
* Description : Starts one measurement (temperature, pressure, humidity, gas) in forced mode.
*
* Argument(s) : none.
*
* Return(s)   : ENV_ERR_NONE or ENV_ERR_SPI.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) The oversampling bits are in the same register as the mode bits, so they are written
*                   again together with mode = forced (ch. 3.5, step 8).
*               (2) The function does not wait. The measurement takes longer than one OS tick (heater
*                   alone 100 ms), so the calling task waits with an OS delay and then calls
*                   env_read_raw() ("Thou shalt release the CPU").
*               (3) After the measurement the sensor goes back to sleep mode by itself (table 12).
*********************************************************************************************************
*/

ENV_ERR env_start_meas(CPU_VOID)
{
    return (env_write_reg(ENV_REG_CTRL_MEAS,                    /* Note 1                               */
                          (CPU_INT08U)((ENV_OSRS_T << ENV_OSRS_T_SHIFT) |
                                       (ENV_OSRS_P << ENV_OSRS_P_SHIFT) |
                                        ENV_MODE_FORCED)));
}

/*
*********************************************************************************************************
*                                           env_read_raw()
*
* Description : Reads the raw measurement data and the status bits of field 0.
*
* Argument(s) : p_raw   structure that receives the raw data.
*
* Return(s)   : ENV_ERR_NONE    data read (check p_raw->new_data before using the values).
*               ENV_ERR_ARG     p_raw is a null pointer.
*               ENV_ERR_SPI     SPI transfer failed.
*
* Caller(s)   : Application, after env_start_meas() and an OS delay.
*
* Note(s)     : (1) All of field 0 is read with one multi-byte read, so status and values belong to the
*                   same measurement (the sensor uses shadow registers, ch. 5.3.5).
*               (2) new_data = DEF_NO means the measurement is not finished yet: wait and read again.
*               (3) gas_valid and heat_stab must both be DEF_YES for a usable gas value (ch. 3.7).
*********************************************************************************************************
*/

ENV_ERR env_read_raw(ENV_RAW *p_raw)
{
    CPU_INT08U  fld[ENV_FIELD0_LEN];
    CPU_INT08U  gas_lsb;
    ENV_ERR     err;


    if (p_raw == (ENV_RAW *)0) {                                /* Check argument                       */
        return (ENV_ERR_ARG);
    }

    err = env_read_regs(ENV_FIELD0_START, &fld[0], ENV_FIELD0_LEN);     /* Read field 0 (Note 1)       */
    if (err != ENV_ERR_NONE) {
        return (err);
    }
                                                                /* Status (Note 2)                      */
    p_raw->new_data  = ((fld[ENV_REG_MEAS_STATUS_0 - ENV_FIELD0_START] & ENV_NEW_DATA) != 0u) ? DEF_YES : DEF_NO;

    p_raw->press_adc = env_u20(fld[ENV_REG_PRESS_MSB  - ENV_FIELD0_START],    /* 20 bit pressure      */
                               fld[ENV_REG_PRESS_LSB  - ENV_FIELD0_START],
                               fld[ENV_REG_PRESS_XLSB - ENV_FIELD0_START]);
    p_raw->temp_adc  = env_u20(fld[ENV_REG_TEMP_MSB   - ENV_FIELD0_START],    /* 20 bit temperature   */
                               fld[ENV_REG_TEMP_LSB   - ENV_FIELD0_START],
                               fld[ENV_REG_TEMP_XLSB  - ENV_FIELD0_START]);
    p_raw->hum_adc   = env_u16(fld[ENV_REG_HUM_MSB    - ENV_FIELD0_START],    /* 16 bit humidity      */
                               fld[ENV_REG_HUM_LSB    - ENV_FIELD0_START]);

    gas_lsb          = fld[ENV_REG_GAS_R_LSB - ENV_FIELD0_START];             /* 10 bit gas + status  */
    p_raw->gas_adc   = (CPU_INT16U)(((CPU_INT16U)fld[ENV_REG_GAS_R_MSB - ENV_FIELD0_START] << ENV_GAS_MSB_SHIFT) |
                                    ((CPU_INT16U)gas_lsb >> ENV_GAS_LSB_SHIFT));
    p_raw->gas_range = (CPU_INT08U)(gas_lsb & ENV_GAS_RANGE_MASK);
    p_raw->gas_valid = ((gas_lsb & ENV_GAS_VALID) != 0u) ? DEF_YES : DEF_NO;  /* Note 3               */
    p_raw->heat_stab = ((gas_lsb & ENV_HEAT_STAB) != 0u) ? DEF_YES : DEF_NO;

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

/*
*********************************************************************************************************
*                                             env_u16()
*
* Description : Combines two register bytes into one 16 bit value.
*
* Argument(s) : msb     most significant byte.
*
*               lsb     least significant byte.
*
* Return(s)   : (msb << 8) | lsb
*
* Caller(s)   : env_read_calib().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static CPU_INT16U env_u16(CPU_INT08U msb, CPU_INT08U lsb)
{
    return ((CPU_INT16U)(((CPU_INT16U)msb << ENV_BYTE_BITS) | lsb));
}

/*
*********************************************************************************************************
*                                             env_u20()
*
* Description : Combines three register bytes into one 20 bit value (temperature, pressure).
*
* Argument(s) : msb     bits 19:12.
*
*               lsb     bits 11:4.
*
*               xlsb    bits 3:0 in xlsb<7:4>.
*
* Return(s)   : (msb << 12) | (lsb << 4) | (xlsb >> 4)
*
* Caller(s)   : env_read_raw().
*
* Note(s)     : (1) Datasheet ch. 5.3.5.1 / 5.3.5.2.
*********************************************************************************************************
*/

static CPU_INT32U env_u20(CPU_INT08U msb, CPU_INT08U lsb, CPU_INT08U xlsb)
{
    return (((CPU_INT32U)msb  << ENV_MSB_SHIFT_20BIT) |
            ((CPU_INT32U)lsb  << ENV_LSB_SHIFT_20BIT) |
            ((CPU_INT32U)xlsb >> ENV_LSB_SHIFT_20BIT));
}

/* [] END OF FILE */