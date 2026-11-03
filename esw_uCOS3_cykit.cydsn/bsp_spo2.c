/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
 */
#include <bsp_spo2.h>
/*
*********************************************************************************************************
*                                       spo2_read_register()
*
* Description : Reads a register from the sensor
*
* Argument(s) : slave_addr ... Adress of the sensor
                reg ... Register Adress
                *value ... Result from read
*
* Return(s)   : spo2_err
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
CPU_INT08U spo2_read_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U *value)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;

    if (value == NULL){
        /*Error handling*/
        spo2_err = SPO2_ERR_VAL;
        return spo2_err;
    }
    /*Starts transmission to slave*/
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    
    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    /*Tells sensor which register it wants to read*/
    err_status = I2C_spo2_MasterWriteByte(reg);

    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    /* Tells the sensor to "remember" which register it wants to read and restarts transmission in read mode. */
    err_status = I2C_spo2_MasterSendRestart(slave_addr, I2C_spo2_READ_XFER_MODE);

    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    /*Reads out register byte*/
    *value = I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);

    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_READ;
        return spo2_err;
    }
    /*Stops transmission*/
    I2C_spo2_MasterSendStop();
    return spo2_err;
}
/*
*********************************************************************************************************
*                                       spo2_write_register()
*
* Description : Writes to register from the sensor
*
* Argument(s) : slave_addr ... Adress of the sensor
                reg ... Register Adress
                config_value ... Value that should be written to register
*
* Return(s)   : spo2_err
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
CPU_INT08U spo2_write_register(CPU_INT08U slave_addr, CPU_INT08U reg, CPU_INT08U config_value)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;
    // Who do you want to write to?
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    // Where do you want to write?
    err_status = I2C_spo2_MasterWriteByte(reg);
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        spo2_err = SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    // What do you want to write?
    err_status = I2C_spo2_MasterWriteByte(config_value);
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
*                                       spo2_check connection()
*
* Description : Checks if a sensor is connected
*
* Argument(s) : slave_addr ... Adress of the sensor
*
* Return(s)   : spo2_err
*
* Caller(s)   : Local.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
static CPU_INT08U spo2_check_connection(CPU_INT08U slave_addr)
{
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;
    /*Starts transmission to sensor*/
    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);

    if (err_status != I2C_spo2_MSTR_NO_ERROR){
        /*If transmission not successful*/
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
* Description : Checks if oximeter 5 click is connected
*
* Argument(s) : slave_addr ... Adress of the sensor
                sensor_reg ... register where sensor id is stored
                sensor_id ... expected sensor id
                *spo2_err ... error pointer
*
* Return(s)   : none
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void Check_Sensor(CPU_INT08U slave_addr, CPU_INT08U sensor_reg, CPU_INT08U sensor_id, SPO2_ERROR *spo2_err)
{
    CPU_INT08U read_id;
    /*Is a sensor connected?*/
    *spo2_err = spo2_check_connection(slave_addr);
    if (*spo2_err != SPO2_ERR_NONE){
        /*Error Handling*/
        return;
    }
    else
    {
        /*If sensor connected check if it is the oximeter 5 click*/
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
* Description : Configures the sensor 
*
* Argument(s) : slave_addr ... Adress of the sensor
                *spo2_err ... error pointer
*
* Return(s)   : none
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void spo2_config(CPU_INT08U slave_addr, SPO2_ERROR *spo2_err)
{
    /*Configures the FIFO*/
    *spo2_err = spo2_write_register(slave_addr, FIFO_Config_Reg, FIFO_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    /*Configures the MODE*/
    *spo2_err = spo2_write_register(slave_addr, MODE_Config_Reg, MODE_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    /*Configures the SPO2*/
    *spo2_err = spo2_write_register(slave_addr, SPO2_Config_Reg, SPO2_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    /*Configures the RED LED*/
    *spo2_err = spo2_write_register(slave_addr, LED_PA_1_Config_Reg, LED_PA_1_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    /*Configures the IR LED*/
    *spo2_err = spo2_write_register(slave_addr, LED_PA_2_Config_Reg, LED_PA_2_Config_Val);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    
    /*Initially sets FIFO Read and Write index to 0*/
    *spo2_err = spo2_write_register(slave_addr, FIFO_RD_PTR_REG, 0x00);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, FIFO_WR_PTR_REG, 0x00);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
    *spo2_err = spo2_write_register(slave_addr, FIFO_OVF_COUNTER_REG, 0x00);
    if (*spo2_err != SPO2_ERR_NONE)
    {
        return;
    }
}
/*
*********************************************************************************************************
*                                       spo2_read_fifo_sample()
*
* Description : Reads one samples out of the fifo
*
* Argument(s) : slave_addr ... Adress of the sensor
                *red ... value of red reading
                *ir ... value of ir reading
                *spo2_err ... error pointer
*
* Return(s)   : none
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir, SPO2_ERROR *spo2_err)
{
    CPU_INT08U err_status;
    CPU_INT08U data[6];

    if ((red == NULL) || (ir == NULL))
    {
        *spo2_err = SPO2_ERR_VAL;
        return;
    }

    err_status = I2C_spo2_MasterSendStart(slave_addr, I2C_spo2_WRITE_XFER_MODE);
    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_SEND;
        return;
    }

    err_status = I2C_spo2_MasterWriteByte(FIFO_DATA_REG);

    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_WRITE;
        return;
    }

    err_status = I2C_spo2_MasterSendRestart(slave_addr, I2C_spo2_READ_XFER_MODE);

    // Checks if error
    if (err_status != I2C_spo2_MSTR_NO_ERROR)
    {
        I2C_spo2_MasterSendStop();
        *spo2_err = SPO2_ERR_MASTER_SEND;
        return;
    }

    for (int i = 0; i <= 4; i++)
    {
        /*Reads the first 5 bytes with ACK (to tell the sensor we want to read more bytes)*/
        data[i] = I2C_spo2_MasterReadByte(I2C_spo2_ACK_DATA);
    }
    /*Reads the last byte with NAK (to tell the sensor we are done reading)*/
    data[5] = I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);
    /*Terminates the transmission*/
    I2C_spo2_MasterSendStop();
    
    /*Bit shifting to extract red and ir value from each byte*/
    *red = (((CPU_INT32U)(data[0] & 0x03u)) << 16) | (((CPU_INT32U)data[1]) << 8) | ((CPU_INT32U)data[2]);

    *ir = (((CPU_INT32U)(data[3] & 0x03u)) << 16) | (((CPU_INT32U)data[4]) << 8) | ((CPU_INT32U)data[5]);
}
/*
*********************************************************************************************************
*                                       sample_available()
*
* Description : Checks if a samples is available
*
* Argument(s) : slave_addr ... Adress of the sensor
                *num_samples_available ... number of samples that are ready
                *spo2_err ... error pointer
*
* Return(s)   : BOOLEAN
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available, SPO2_ERROR *spo2_err)
{
    CPU_INT08U read_idx;
    CPU_INT08U write_idx;
    CPU_INT08U ovf;
    /*Checks read and write value*/
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

    /*If read is smaller than write, then there are samples available*/
    if (read_idx < write_idx)
    {
        *num_samples_available = write_idx - read_idx;
        return 1;
    }
    /*If read is bigger than write, there are samples available, but the buffer has looped around*/
    if (read_idx > write_idx)
    {
        *num_samples_available = FIFO_SAMPLE_BUFFER - read_idx + write_idx;
        return 1;
    }
    /*If they are the same, no new samples*/
    if (read_idx == write_idx)
    {
        return 0;
    }
    return 0;
}
/*
*********************************************************************************************************
*                                       spo2_sleep()
*
* Description : Sends the sensor to sleep mode
*
* Argument(s) : none
*
* Return(s)   : spo2_error
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
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
* Description : Wakes the sensor
*
* Argument(s) : none
*
* Return(s)   : spo2_error
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
SPO2_ERROR spo2_wake(void) {

    SPO2_ERROR err;

    // Reset FIFO
    err = spo2_write_register(SPO2_ADDRESS, FIFO_RD_PTR_REG, 0x00);
    if(err != SPO2_ERR_NONE) return err;

    err = spo2_write_register(SPO2_ADDRESS, FIFO_WR_PTR_REG, 0x00);
    if(err != SPO2_ERR_NONE) return err;

    err = spo2_write_register(SPO2_ADDRESS, FIFO_OVF_COUNTER_REG, 0x00);
    if(err != SPO2_ERR_NONE) return err;

    // Wake Sensor
    return spo2_write_register(SPO2_ADDRESS,
                               MODE_Config_Reg,
                               MODE_Config_Val);
}

/* [] END OF FILE */
