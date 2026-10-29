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
    SPO2_ERROR spo2_err=SPO2_ERR_NONE;
    
    if(value == NULL){
        spo2_err=SPO2_ERR_VAL;
        return spo2_err;
    }
    
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    
    err_status=I2C_spo2_MasterWriteByte(reg);
    
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    
    err_status=I2C_spo2_MasterSendRestart(slave_addr,I2C_spo2_READ_XFER_MODE);
    
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    
    *value=I2C_spo2_MasterReadByte(I2C_spo2_NAK_DATA);
    
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_READ;
        return spo2_err;
    }
    
    I2C_spo2_MasterSendStop();
    return spo2_err;
}

CPU_INT08U spo2_write_register(CPU_INT08U slave_addr,CPU_INT08U reg, CPU_INT08U config_value){
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err=SPO2_ERR_NONE;
    //Who do you want to write to?
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_SEND;
        return spo2_err;
    }
    //Where do you want to write?
    err_status=I2C_spo2_MasterWriteByte(reg);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    //What do you want to write?
    err_status=I2C_spo2_MasterWriteByte(config_value);
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        spo2_err=SPO2_ERR_MASTER_WRITE;
        return spo2_err;
    }
    I2C_spo2_MasterSendStop();
    return spo2_err;
}

CPU_INT08U spo2_check_connection(CPU_INT08U slave_addr){
    CPU_INT08U err_status;
    SPO2_ERROR spo2_err=SPO2_ERR_NONE;
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){ 
        I2C_spo2_Stop(); 
        I2C_spo2_MasterSendStop(); 
        return SPO2_ERR_NO_CONNECTION;
    }
    
    I2C_spo2_MasterSendStop();
    return spo2_err;
}
void Check_Sensor(CPU_INT08U slave_addr,CPU_INT08U sensor_reg,CPU_INT08U sensor_id,SPO2_ERROR *spo2_err){
    CPU_INT08U read_id;
    *spo2_err=spo2_check_connection(slave_addr);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
        
    }else{
        *spo2_err=spo2_read_register(slave_addr,sensor_reg,&read_id);
        if(*spo2_err!=SPO2_ERR_NONE) {
            return;
            }else{
                if(read_id!=sensor_id){
                    *spo2_err=SPO2_ERR_NO_CONNECTION;
                    return;
                }
            }
        }
}

void spo2_config(CPU_INT08U slave_addr,SPO2_ERROR *spo2_err){

    *spo2_err=spo2_write_register(slave_addr,FIFO_Config_Reg,FIFO_Config_Val);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,MODE_Config_Reg,MODE_Config_Val);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,SPO2_Config_Reg,SPO2_Config_Val);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,LED_PA_1_Config_Reg,LED_PA_1_Config_Val);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,LED_PA_2_Config_Reg,LED_PA_2_Config_Val);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    
    *spo2_err=spo2_write_register(slave_addr,FIFO_RD_PTR_REG,0x00);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,FIFO_WR_PTR_REG,0x00);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    *spo2_err=spo2_write_register(slave_addr,FIFO_OVF_COUNTER_REG,0x00);
    if(*spo2_err!=SPO2_ERR_NONE){
        return;
    }
    
}

void spo2_read_fifo_sample(CPU_INT08U slave_addr, CPU_INT32U *red, CPU_INT32U *ir,SPO2_ERROR *spo2_err){
    CPU_INT08U err_status;
    CPU_INT08U data[6];
    
    if ((red == NULL) || (ir == NULL))
    {
        *spo2_err=SPO2_ERR_VAL;
        return;
    }
    
    err_status=I2C_spo2_MasterSendStart(slave_addr,I2C_spo2_WRITE_XFER_MODE);
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        *spo2_err=SPO2_ERR_MASTER_SEND;
        return;
    }
    
    err_status=I2C_spo2_MasterWriteByte(FIFO_DATA_REG);
    
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        *spo2_err=SPO2_ERR_MASTER_WRITE;
        return;
    }
    
    err_status=I2C_spo2_MasterSendRestart(slave_addr,I2C_spo2_READ_XFER_MODE);
    
    //Checks if error
    if(err_status!=I2C_spo2_MSTR_NO_ERROR){
        I2C_spo2_MasterSendStop();
        *spo2_err=SPO2_ERR_MASTER_SEND;
        return;
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

    
    
}

CPU_BOOLEAN sample_available(CPU_INT08U slave_addr, CPU_INT08U *num_samples_available,SPO2_ERROR *spo2_err){
    CPU_INT08U read_idx;
    CPU_INT08U write_idx;
    CPU_INT08U ovf;
   
    
    *spo2_err=spo2_read_register(slave_addr,FIFO_RD_PTR_REG,&read_idx);
    if(*spo2_err!=SPO2_ERR_NONE){
        return 0;
    }
    *spo2_err=spo2_read_register(slave_addr,FIFO_WR_PTR_REG,&write_idx);
    if(*spo2_err!=SPO2_ERR_NONE){
        return 0;
    }
    *spo2_err=spo2_read_register(slave_addr,FIFO_OVF_COUNTER_REG,&ovf);
    if(*spo2_err!=SPO2_ERR_NONE){
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
