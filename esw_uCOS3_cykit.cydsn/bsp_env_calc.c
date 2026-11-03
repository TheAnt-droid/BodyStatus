/*
*********************************************************************************************************
*                                ENVIRONMENT 3 CLICK (BME688) - CALCULATIONS
*
* Filename      : bsp_env_calc.c
*
* Description   : Integer formulas of the BME688 datasheet (BST-BME688-DS000-00 Rev. 1.0, ch. 3.6, 3.7).
*
* Note(s)       : (1) The integer variants of the datasheet formulas are used: the Cortex-M3 has no
*                     floating point unit, so double/float would be emulated in software (slow, more stack).
*                 (2) The numbers inside the formulas (e.g. 784, 154009, 3276800) are the constants of the
*                     datasheet formulas. They are kept exactly as printed so that every line can be
*                     compared 1:1 with the datasheet; giving them names would not explain them better.
*                 (3) Re-entrant: only local variables, no SPI access.
*********************************************************************************************************
*/

#include <bsp_env_calc.h>

/*
*********************************************************************************************************
*                                        env_calc_res_heat()
*
* Description : Converts a heater target temperature into the register code for res_heat_x.
*
* Argument(s) : p_calib         calibration values (par_g1, par_g2, par_g3, res_heat_range, res_heat_val).
*
*               target_degc     heater target temperature in degC (typically 200 ... 400).
*
*               amb_degc        ambient temperature in degC.
*
* Return(s)   : Register code for res_heat_x (0 if p_calib is a null pointer).
*
* Caller(s)   : env_config().
*
* Note(s)     : (1) Integer formula of datasheet ch. 3.6, "Gas sensor heating and measurement",
*                   table 16. Variable names as in the datasheet.
*               (2) Value range check with typical calibration values: the largest intermediate value
*                   (var2) is about 4e8 and fits into a signed 32 bit integer (max. 2.1e9).
*               (3) The datasheet writes "<< 8" and ">> 1". Here "* 256" and "/ 2" are used: shifting a
*                   negative signed value is not well defined in C (var1 is negative if par_g3 < 0).
*                   For the positive values that occur in practice the result is the same.
*********************************************************************************************************
*/

CPU_INT08U env_calc_res_heat(const ENV_CALIB *p_calib, CPU_INT32S target_degc, CPU_INT32S amb_degc)
{
    CPU_INT32S  var1;
    CPU_INT32S  var2;
    CPU_INT32S  var3;
    CPU_INT32S  var4;
    CPU_INT32S  var5;
    CPU_INT32S  res_heat_x100;


    if (p_calib == (const ENV_CALIB *)0) {                      /* Check argument                       */
        return (0u);
    }
                                                                /* Datasheet formula (Notes 1, 2)       */
    var1 = ((amb_degc * (CPU_INT32S)p_calib->par_g3) / 10) * 256;               /* "<< 8" of the sheet */
    var2 = ((CPU_INT32S)p_calib->par_g1 + 784) *
           (((((((CPU_INT32S)p_calib->par_g2 + 154009) * target_degc * 5) / 100) + 3276800) / 10));
    var3 = var1 + (var2 / 2);                                                   /* "var2 >> 1"         */
    var4 = var3 / ((CPU_INT32S)p_calib->res_heat_range + 4);
    var5 = (131 * (CPU_INT32S)p_calib->res_heat_val) + 65536;
    res_heat_x100 = ((var4 / var5) - 250) * 34;

    return ((CPU_INT08U)((res_heat_x100 + 50) / 100));
}

/* [] END OF FILE */