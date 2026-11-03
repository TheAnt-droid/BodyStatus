/*
*********************************************************************************************************
*                                    ENVIRONMENT 3 CLICK (BME688) - DRIVER
*
* Filename      : bsp_env.h
*
* Description   : Register access to the BME688 via SPI (mikroBUS slot 1 of the Click Shield).
*********************************************************************************************************
*/

#ifndef BSP_ENV_H
#define BSP_ENV_H

#include <cpu.h>
#include <bsp_spi.h>
#include <bsp_env_cfg.h>

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

ENV_ERR env_read_regs   (CPU_INT08U reg, CPU_INT08U *p_buf, CPU_INT08U len);
ENV_ERR env_write_reg   (CPU_INT08U reg, CPU_INT08U val);
ENV_ERR env_check_id    (CPU_VOID);
ENV_ERR env_read_calib  (ENV_CALIB *p_calib);

#endif
/* [] END OF FILE */