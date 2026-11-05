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
#include <bsp_env_cfg.h>

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

ENV_ERR env_check_id    (CPU_VOID);
ENV_ERR env_read_calib  (ENV_CALIB *p_calib);
ENV_ERR env_config      (const ENV_CALIB *p_calib);
ENV_ERR env_start_meas  (CPU_VOID);
ENV_ERR env_read_raw    (ENV_RAW *p_raw);

#endif
/* [] END OF FILE */
