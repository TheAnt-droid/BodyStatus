/*
*********************************************************************************************************
*                                  OXIMETER 5 CLICK (MAX30102) - DRIVER
*
* Filename      : bsp_spo2.c
*
* Description   : I2C access to the MAX30102 on the Oximeter 5 Click (I2C_spo2, mikroBUS slot 2): register
*                 read/write, sensor check, configuration, reading the FIFO, sleep and wake.
*                 Pins (Click Shield): SCL P12_0, SDA P12_1
*
* Note(s)       : (1) Not re-entrant (I2C bus). The driver is only used by main() before the OS starts and
*                     by App_TaskSPO2(), so no mutex is needed.
*********************************************************************************************************
*/
#include <bsp_spo2.h>
/*
*********************************************************************************************************
*                                       spo2_read_register()
*
* Description : Reads one register of the sensor.
*
* Argument(s) : slave_addr  I2C address of the sensor.
*
*               reg         register address.
*
*               value       receives the register value.
*
* Return(s)   : SPO2_ERR_NONE, or the error of the failed I2C step.
*
* Caller(s)   : Check_Sensor(), sample_available()
*
* Note(s)     : none.
*********************************************************************************************************
*/
CPU_INT08U spo2_read_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U *value)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;

    if (value == NULL){                                         /* No target for the value              */
        spo2_err = SPO2_ERR_VAL;
        return spo2_err;
    }
                                                                /* Start, slave address in write mode   */
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    
    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    err_status = I2C_spo2_MasterWriteByte(reg);                 /* Register address                     */

    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
                                                                /* Repeated start in read mode, the     */
                                                                /* sensor keeps the register address    */
    err_status = I2C_spo2_MasterSendRestart(slave_addr, I2C_spo2_READ_XFER_MODE);

    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    *value = I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);        /* Read one byte, NAK = last byte       */

    I2C_spo2_MasterSendStop();
    return spo2_err;
}
/*
*********************************************************************************************************
*                                       spo2_write_register()
*
* Description : Writes one register of the sensor.
*
* Argument(s) : slave_addr    I2C address of the sensor.
*
*               reg           register address.
*
*               config_value  value to write.
*
* Return(s)   : SPO2_ERR_NONE, or the error of the failed I2C step.
*
* Caller(s)   : spo2_config(), spo2_sleep(), spo2_wake()
*
* Note(s)     : none.
*********************************************************************************************************
*/
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U config_value)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;
                                                                /* Start, slave address in write mode   */
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    err_status = I2C_spo2_MasterWriteByte(reg);                 /* Register address                     */
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    err_status = I2C_spo2_MasterWriteByte(config_value);        /* Value                                */
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    I2C_spo2_MasterSendStop();
    return spo2_err;
}
/*
*********************************************************************************************************
*                                       spo2_check_connection()
*
* Description : Checks whether a device answers on the I2C address.
*
* Argument(s) : slave_addr  I2C address of the sensor.
*
* Return(s)   : SPO2_ERR_NONE, or SPO2_ERR_NO_CONNECTION (the I2C component is stopped then).
*
* Caller(s)   : Check_Sensor()
*
* Note(s)     : none.
*********************************************************************************************************
*/
static CPU_INT08U spo2_check_connection(CPU_INT08U slave_addr)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;
                                                                /* Start: does a device acknowledge the */
                                                                /* address?                             */
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);

    if (err_status != I2C_spo2_MSTR_NO_ERROR){                  /* No answer                            */
        I2C_spo2_Stop();
        I2C_spo2_MasterSendStop();
        return SPO2_ERR_NO_CONNECTION;
    }
    
    I2C_spo2_MasterSendStop();
    return spo2_err;
}
/*
*********************************************************************************************************
*                                       Check_Sensor()
*
* Description : Checks whether the Oximeter 5 Click is connected: the device answers and its ID register
*               holds the expected ID.
*
* Argument(s) : slave_addr  I2C address of the sensor.
*
*               sensor_reg  register with the ID.
*
*               sensor_id   expected ID.
*
*               spo2_err    receives SPO2_ERR_NONE or the error.
*
* Return(s)   : none
*
* Caller(s)   : main()
*
* Note(s)     : none.
*********************************************************************************************************
*/
void Check_Sensor(CPU_INT08U slave_addr, CPU_INT08U sensor_reg, CPU_INT08U sensor_id, SPO2_ERROR *spo2_err)
{
    CPU_INT08U read_id;
    *spo2_err = spo2_check_connection(slave_addr);              /* Does a device answer?                */
    if (*spo2_err != SPO2_ERR_NONE){                            /* No device                            */
        return;
    }
    else                                                        /* Device answers: is it the MAX30102?  */
    {
        *spo2_err = spo2_read_register(slave_addr, sensor_reg, &read_id);
        if (*spo2_err != SPO2_ERR_NONE)
        {
            return;
        }
        else
        {
            if (read_id != sensor_id)
            {
                *spo2_err = SPO2_ERR_NO_CONNECTION;
                return;
            }
        }
    }
}
/*
*********************************************************************************************************
*                                       spo2_config()
*
* Description : Configures FIFO, mode, SpO2 settings and LED currents (bsp_oximeter_cfg.h) and sets the
*               FIFO indexes to 0.
*
* Argument(s) : slave_addr  I2C address of the sensor.
*
*               spo2_err    receives SPO2_ERR_NONE or the error of the first failed write.
*
* Return(s)   : none
*
* Caller(s)   : main()
*
* Note(s)     : none.
*********************************************************************************************************
*/
void spo2_config(CPU_INT08U slave_addr, SPO2_ERROR *spo2_err)
{
    *spo2_err = spo2_write_register(slave_addr, FIFO_Config_Reg, FIFO_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, MODE_Config_Reg, MODE_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, SPO2_Config_Reg, SPO2_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, LED_PA_1_Config_Reg, LED_PA_1_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, LED_PA_2_Config_Reg, LED_PA_2_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    
                                                                /* Start with an empty FIFO: read/write */
                                                                /* index and overflow counter = 0       */
    *spo2_err = spo2_write_register(slave_addr, FIFO_RD_PTR_REG, FIFO_PTR_RESET);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, FIFO_WR_PTR_REG, FIFO_PTR_RESET);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, FIFO_OVF_COUNTER_REG, FIFO_PTR_RESET);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
}
/*
*********************************************************************************************************
*                                       spo2_read_fifo_sample()
*
* Description : Reads one sample (red and IR, 3 bytes each) from the FIFO.
*
* Argument(s) : slave_addr  I2C address of the sensor.
*
*               red         receives the red value.
*
*               ir          receives the IR value.
*
*               spo2_err    receives the error. Not changed on success, so the caller sets it first.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2()
*
* Note(s)     : none.
*********************************************************************************************************
*/
void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir, SPO2_ERROR *spo2_err)
{
    CPU_INT08U err_status;
    CPU_INT08U data[FIFO_SAMPLE_BYTES];

    if ((red == NULL) || (ir == NULL))
    {
        *spo2_err = SPO2_ERR_VAL;
        return;
    }

    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_SEND;
        return;
    }

    err_status = I2C_spo2_MasterWriteByte(FIFO_DATA_REG);

    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_WRITE;
        return;
    }

    err_status = I2C_spo2_MasterSendRestart(slave_addr, I2C_spo2_READ_XFER_MODE);

    if (err_status != I2C_spo2_MSTR_NO_ERROR)                   /* Error: stop the transfer             */
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_SEND;
        return;
    }

    for (CPU_INT08U i = 0u; i < (FIFO_SAMPLE_BYTES - 1u); i++)  /* First 5 bytes with ACK: more bytes   */
    {                                                           /* follow                               */
        data[i] = I2C_spo2_MasterReadByte(I2C_spo2_ACK_DATA);
    }
                                                                /* Last byte with NAK: end of the read  */
    data[FIFO_SAMPLE_BYTES - 1u] = I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);
    I2C_spo2_MasterSendStop();
    
                                                                /* 3 bytes per value, only the lower 18 */
                                                                /* bits are data (18 bit ADC,           */
                                                                /* SPO2_Config_Val)                     */
    *red = (((CPU_INT32U)(data[FIFO_RED_IDX + FIFO_BYTE_MSB] & FIFO_MSB_MASK)) << FIFO_MSB_SHIFT) |
           (((CPU_INT32U) data[FIFO_RED_IDX + FIFO_BYTE_MID]) << FIFO_MID_SHIFT) |
            ((CPU_INT32U) data[FIFO_RED_IDX + FIFO_BYTE_LSB]);

    *ir  = (((CPU_INT32U)(data[FIFO_IR_IDX + FIFO_BYTE_MSB] & FIFO_MSB_MASK)) << FIFO_MSB_SHIFT) |
           (((CPU_INT32U) data[FIFO_IR_IDX + FIFO_BYTE_MID]) << FIFO_MID_SHIFT) |
            ((CPU_INT32U) data[FIFO_IR_IDX + FIFO_BYTE_LSB]);
}
/*
*********************************************************************************************************
*                                       sample_available()
*
* Description : Checks with the FIFO read and write index whether new samples are in the FIFO.
*
* Argument(s) : slave_addr              I2C address of the sensor.
*
*               num_samples_available   receives the number of new samples (only set if there are any).
*
*               spo2_err                receives SPO2_ERR_NONE or the error.
*
* Return(s)   : 1 if there are new samples, otherwise 0 (also on an error).
*
* Caller(s)   : App_TaskSPO2()
*
* Note(s)     : none.
*********************************************************************************************************
*/
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available, SPO2_ERROR *spo2_err)
{
    CPU_INT08U read_idx;
    CPU_INT08U write_idx;
                                                                /* Read and write index of the FIFO     */
    *spo2_err = spo2_read_register(slave_addr, FIFO_RD_PTR_REG, &read_idx);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return 0;
    }
    *spo2_err = spo2_read_register(slave_addr, FIFO_WR_PTR_REG, &write_idx);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return 0;
    }

    if (read_idx < write_idx)                                   /* Read < write: new samples            */
    {
        *num_samples_available = write_idx - read_idx;
        return 1;
    }
                                                                /* Read > write: new samples, the write */
                                                                /* index has wrapped around             */
    if (read_idx > write_idx)
    {
        *num_samples_available = FIFO_SAMPLE_BUFFER - read_idx + write_idx;
        return 1;
    }
    if (read_idx == write_idx)                                  /* Read == write: no new samples        */
    {
        return 0;
    }
    return 0;
}
/*
*********************************************************************************************************
*                                       spo2_sleep()
*
* Description : Puts the sensor into shutdown mode (LEDs off) while nothing is measured.
*
* Argument(s) : none
*
* Return(s)   : SPO2_ERR_NONE or the error of spo2_write_register().
*
* Caller(s)   : main(), App_TaskSPO2()
*
* Note(s)     : none.
*********************************************************************************************************
*/
SPO2_ERROR spo2_sleep(void) {
    return spo2_write_register(SPO2_ADDRESS,
                               MODE_Config_Reg,
                               MODE_Config_Val_Sleep);
}
/*
*********************************************************************************************************
*                                       spo2_wake()
*
* Description : Clears the FIFO indexes and wakes the sensor up, so the measurement starts without old
*               samples.
*
* Argument(s) : none
*
* Return(s)   : SPO2_ERR_NONE or the error of the first failed write.
*
* Caller(s)   : App_TaskSPO2()
*
* Note(s)     : none.
*********************************************************************************************************
*/
SPO2_ERROR spo2_wake(void) {

    SPO2_ERROR err;

                                                                /* Start with an empty FIFO             */
    err = spo2_write_register(SPO2_ADDRESS, FIFO_RD_PTR_REG, FIFO_PTR_RESET);
    if(err != SPO2_ERR_NONE) return err;

    err = spo2_write_register(SPO2_ADDRESS, FIFO_WR_PTR_REG, FIFO_PTR_RESET);
    if(err != SPO2_ERR_NONE) return err;

    err = spo2_write_register(SPO2_ADDRESS, FIFO_OVF_COUNTER_REG, FIFO_PTR_RESET);
    if(err != SPO2_ERR_NONE) return err;

    return spo2_write_register(SPO2_ADDRESS,                    /* Wake up                              */
                               MODE_Config_Reg,
                               MODE_Config_Val);
}

/* [] END OF FILE */
