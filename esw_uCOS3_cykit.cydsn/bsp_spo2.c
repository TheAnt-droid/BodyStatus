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



CPU_INT08U spo2_read_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U *value){
    CPU_INT08U err_status;
    if(value == NULL){
    return 1;
    }
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    err_status=I2C_spo2_MasterWriteByte(reg);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    err_status=I2C_spo2_MasterSendRestart(slave_addr,I2C_spo2_READ_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    *value=I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);
    I2C_spo2_MasterSendStop();
    return err_status;
}

CPU_INT08S spo2_check_connection(CPU_INT08U slave_addr){
    CPU_INT08U err_status;
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){ I2C_spo2_Stop(); I2C_spo2_MasterSendStop(); return I2C_spo2_MSTR_ERR_ARB_LOST;}
    
    I2C_spo2_MasterSendStop();
    return err_status;
}
void Check_Sensor(CPU_INT08U I2C_addr,CPU_INT08U sensor_reg,CPU_INT08U sensor_id){
    CPU_INT08U read_id;
    if(spo2_check_connection(I2C_addr)!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_Stop();
    }else{
        if(spo2_read_register(I2C_addr,sensor_reg,&read_id)!=I2C_spo2_MSTR_NO_ERROR) {
            I2C_spo2_Stop();
            }else{
                if(read_id!=sensor_id){
                I2C_spo2_Stop();
                }
            }
        }
}





/* [] END OF FILE */
