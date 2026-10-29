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

CPU_INT08U spo2_write_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U config_value){
    CPU_INT08U err_status;
    //Who do you want to write to?
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    //Where do you want to write?
    err_status=I2C_spo2_MasterWriteByte(reg);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    //What do you want to write?
    err_status=I2C_spo2_MasterWriteByte(config_value);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    I2C_spo2_MasterSendStop();
    return I2C_spo2_MSTR_NO_ERROR;
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

CPU_INT08U spo2_config(CPU_INT08U I2C_addr){
    CPU_INT08U err_status;
    err_status=spo2_write_register(I2C_addr,FIFO_Reg,FIFO_Config);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
    return err_status;
    }
    err_status=spo2_write_register(I2C_addr,MODE_Reg,MODE_Config);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
    return err_status;
    }
    err_status=spo2_write_register(I2C_addr,SPO2_Reg,SPO2_Config);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
    return err_status;
    }
    err_status=spo2_write_register(I2C_addr,LED_PA_1_Reg,LED_PA_1_Config);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
    return err_status;
    }
    err_status=spo2_write_register(I2C_addr,LED_PA_2_Reg,LED_PA_2_Config);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
    return err_status;
    }
    
    spo2_write_register(I2C_addr,FIFO_RD_PTR,0x00);
    spo2_write_register(I2C_addr,FIFO_WR_PTR,0x00);
    spo2_write_register(I2C_addr,FIFO_OVF_COUNTER,0x00);
    
    return I2C_spo2_MSTR_NO_ERROR;
}

CPU_INT08U spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir){
    CPU_INT08U err_status;
    CPU_INT08U data[6];
    
    if ((red == NULL) || (ir == NULL))
    {
        return 1u;
    }
    
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    
    err_status=I2C_spo2_MasterWriteByte(FIFO_DATA);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    
    err_status=I2C_spo2_MasterSendRestart(slave_addr,I2C_spo2_READ_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        return err_status;
    }
    
    for(int i=0;i<=4;i++){
        data[i]=I2C_spo2_MasterReadByte(I2C_spo2_ACK_DATA);
    }
    data[5]=I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);
    I2C_spo2_MasterSendStop();
    
     *red = (((CPU_INT32U)(data[0] & 0x03u)) << 16)
         | (((CPU_INT32U)data[1]) << 8)
         | ((CPU_INT32U)data[2]);

    *ir  = (((CPU_INT32U)(data[3] & 0x03u)) << 16)
         | (((CPU_INT32U)data[4]) << 8)
         | ((CPU_INT32U)data[5]);

    return I2C_spo2_MSTR_NO_ERROR;
    
}

CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available){
    CPU_INT08U read_idx;
    CPU_INT08U write_idx;
    CPU_INT08U ovf;
    CPU_INT08U error_status;
    error_status=spo2_read_register(slave_addr,FIFO_RD_PTR,&read_idx);
    if(error_status!=I2C_spo2_MSTR_NO_ERROR){
        return 0;
    }
    error_status=spo2_read_register(slave_addr,FIFO_WR_PTR,&write_idx);
    if(error_status!=I2C_spo2_MSTR_NO_ERROR){
        return 0;
    }
    error_status=spo2_read_register(slave_addr,FIFO_OVF_COUNTER,&ovf);
    if(error_status!=I2C_spo2_MSTR_NO_ERROR){
        return 0;
    }
    
        if(read_idx<write_idx){
            *num_samples_available=write_idx-read_idx; 
            return 1;
        }
        if(read_idx>write_idx){
            *num_samples_available=FIFO_SAMPLE_BUFFER-read_idx+write_idx;
            return 1;
        }
        if(read_idx==write_idx){
            return 0;
        }
    return 0;
}
/* [] END OF FILE */
