/*
*********************************************************************************************************
*                                ENVIRONMENT 3 CLICK (BME688) - CALCULATIONS
*
* Filename      : bsp_env_calc.h
*
* Description   : Integer formulas of the BME688 datasheet (BST-BME688-DS000-00 Rev. 1.0, ch. 3.6, 3.7).
*                 No SPI access here: the functions only calculate with values passed to them.
*********************************************************************************************************
*/

#ifndef BSP_ENV_CALC_H
#define BSP_ENV_CALC_H

#include <cpu.h>
#include <lib_def.h>
#include <bsp_env_cfg.h>

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_INT08U  env_calc_res_heat (const ENV_CALIB *p_calib, CPU_INT32S target_degc, CPU_INT32S amb_degc);
CPU_BOOLEAN env_calc_data     (const ENV_CALIB *p_calib, const ENV_RAW *p_raw, ENV_DATA *p_data);

#endif
/* [] END OF FILE */
