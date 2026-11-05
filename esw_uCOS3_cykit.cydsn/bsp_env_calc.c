/*
*********************************************************************************************************
*                                ENVIRONMENT 3 CLICK (BME688) - CALCULATIONS
*
* Filename      : bsp_env_calc.c
*
* Description   : Integer formulas of the BME688 datasheet (BST-BME688-DS000-00 Rev. 1.0, ch. 3.6, 3.7):
*                 raw values -> degC, Pa, %rH, Ohm, and heater temperature -> register code.
*
* Note(s)       : (1) Integer instead of float formulas: the Cortex-M3 has no FPU, float would be
*                     emulated in software (slow, more stack).
*                 (2) The numbers inside the formulas (e.g. 784, 154009) are the datasheet constants. They
*                     stay as printed, so every line can be compared 1:1 with the datasheet; a name would
*                     not explain them better.
*                 (3) Re-entrant: only local variables, no SPI access.
*                 (4) Shifts: ">>" of signed values is kept (ARM GCC shifts arithmetically, as the formulas
*                     expect). "<<" of values that can be negative is written as a multiplication,
*                     because a left shift of a negative value is undefined in C.
*********************************************************************************************************
*/

#include <bsp_env_calc.h>

/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static CPU_INT32S env_calc_temp  (const ENV_CALIB *p_calib, CPU_INT32U temp_adc, CPU_INT32S *p_t_fine);
static CPU_INT32U env_calc_press (const ENV_CALIB *p_calib, CPU_INT32U press_adc, CPU_INT32S t_fine);
static CPU_INT32U env_calc_hum   (const ENV_CALIB *p_calib, CPU_INT16U hum_adc, CPU_INT32S temp_c100);
static CPU_INT32U env_calc_gas   (CPU_INT16U gas_adc, CPU_INT08U gas_range);

/*
*********************************************************************************************************
*                                        env_calc_res_heat()
*
* Description : Converts a heater target temperature into the register code for res_heat_x (ch. 3.6).
*
* Argument(s) : p_calib         calibration values (par_g1..3, res_heat_range, res_heat_val).
*
*               target_degc     heater target temperature in degC.
*
*               amb_degc        ambient temperature in degC.
*
* Return(s)   : Register code (0 if p_calib is a null pointer).
*
* Caller(s)   : env_config().
*
* Note(s)     : (1) "<< 8" -> "* 256" and ">> 1" -> "/ 2" (file Note 4).
*               (2) The largest intermediate value (var2, about 4e8) fits into CPU_INT32S.
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

    var1 = ((amb_degc * (CPU_INT32S)p_calib->par_g3) / 10) * 256;               /* Note 1              */
    var2 = ((CPU_INT32S)p_calib->par_g1 + 784) *                                /* Note 2              */
           (((((((CPU_INT32S)p_calib->par_g2 + 154009) * target_degc * 5) / 100) + 3276800) / 10));
    var3 = var1 + (var2 / 2);                                                   /* Note 1              */
    var4 = var3 / ((CPU_INT32S)p_calib->res_heat_range + 4);
    var5 = (131 * (CPU_INT32S)p_calib->res_heat_val) + 65536;
    res_heat_x100 = ((var4 / var5) - 250) * 34;

    return ((CPU_INT08U)((res_heat_x100 + 50) / 100));
}

/*
*********************************************************************************************************
*                                          env_calc_data()
*
* Description : Converts the raw values of one measurement into temperature, pressure, humidity and gas
*               resistance.
*
* Argument(s) : p_calib     calibration values.
*
*               p_raw       raw values of one measurement.
*
*               p_data      structure that receives the converted values.
*
* Return(s)   : DEF_OK      values calculated.
*               DEF_FAIL    null pointer or no new data.
*
* Caller(s)   : App_EnvMeasure().
*
* Note(s)     : (1) Temperature first: pressure needs t_fine, humidity the compensated temperature.
*               (2) The gas value is only usable if gas_valid and heat_stab are both set (ch. 3.7).
*********************************************************************************************************
*/

CPU_BOOLEAN env_calc_data(const ENV_CALIB *p_calib, const ENV_RAW *p_raw, ENV_DATA *p_data)
{
    CPU_INT32S  t_fine;


    if ((p_calib == (const ENV_CALIB *)0) ||                    /* Check arguments                      */
        (p_raw   == (const ENV_RAW   *)0) ||
        (p_data  == (ENV_DATA        *)0)) {
        return (DEF_FAIL);
    }
    if (p_raw->new_data != DEF_YES) {                           /* Only complete measurements           */
        return (DEF_FAIL);
    }

    p_data->temp_c100 = env_calc_temp(p_calib, p_raw->temp_adc, &t_fine);          /* Note 1           */
    p_data->press_pa  = env_calc_press(p_calib, p_raw->press_adc, t_fine);
    p_data->hum_m1000 = env_calc_hum(p_calib, p_raw->hum_adc, p_data->temp_c100);
    p_data->gas_ohm   = env_calc_gas(p_raw->gas_adc, p_raw->gas_range);
    p_data->gas_ok    = ((p_raw->gas_valid == DEF_YES) &&                          /* Note 2           */
                         (p_raw->heat_stab == DEF_YES)) ? DEF_YES : DEF_NO;

    return (DEF_OK);
}

/*
*********************************************************************************************************
*                                          env_calc_temp()
*
* Description : Temperature compensation (ch. 3.6, table 13).
*
* Argument(s) : p_calib     calibration values (par_t1..3).
*
*               temp_adc    raw temperature (20 bit).
*
*               p_t_fine    receives t_fine (intermediate value, needed for the pressure).
*
* Return(s)   : Temperature in 0.01 degC.
*
* Caller(s)   : env_calc_data().
*
* Note(s)     : (1) "par_t3 << 4" -> "* 16" (file Note 4).
*********************************************************************************************************
*/

static CPU_INT32S env_calc_temp(const ENV_CALIB *p_calib, CPU_INT32U temp_adc, CPU_INT32S *p_t_fine)
{
    CPU_INT32S  var1;
    CPU_INT32S  var2;
    CPU_INT32S  var3;
    CPU_INT32S  t_fine;


    var1   = ((CPU_INT32S)temp_adc >> 3) - ((CPU_INT32S)p_calib->par_t1 * 2);
    var2   = (var1 * (CPU_INT32S)p_calib->par_t2) >> 11;
    var3   = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((CPU_INT32S)p_calib->par_t3 * 16)) >> 14;    /* Note 1 */
    t_fine = var2 + var3;

    *p_t_fine = t_fine;
    return (((t_fine * 5) + 128) >> 8);
}

/*
*********************************************************************************************************
*                                          env_calc_press()
*
* Description : Pressure compensation (ch. 3.6, table 14).
*
* Argument(s) : p_calib     calibration values (par_p1..10).
*
*               press_adc   raw pressure (20 bit).
*
*               t_fine      intermediate value of the temperature formula.
*
* Return(s)   : Pressure in Pa.
*
* Caller(s)   : env_calc_data().
*
* Note(s)     : (1) "<< n" of signed parameters -> "* 2^n" (file Note 4).
*               (2) press_comp is unsigned as in the datasheet: "* 3125" would overflow a signed value.
*               (3) var1 = 0 only with invalid calibration data; protects the division.
*********************************************************************************************************
*/

static CPU_INT32U env_calc_press(const ENV_CALIB *p_calib, CPU_INT32U press_adc, CPU_INT32S t_fine)
{
    CPU_INT32S  var1;
    CPU_INT32S  var2;
    CPU_INT32S  var3;
    CPU_INT32U  press_comp;


    var1 = (t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (CPU_INT32S)p_calib->par_p6) >> 2;
    var2 = var2 + ((var1 * (CPU_INT32S)p_calib->par_p5) * 2);                                 /* Note 1 */
    var2 = (var2 >> 2) + ((CPU_INT32S)p_calib->par_p4 * 65536);                               /* Note 1 */
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) *
             ((CPU_INT32S)p_calib->par_p3 * 32)) >> 3) +                                       /* Note 1 */
           (((CPU_INT32S)p_calib->par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (CPU_INT32S)p_calib->par_p1) >> 15;

    if (var1 == 0) {                                            /* Note 3                               */
        return (0u);
    }

    press_comp = (CPU_INT32U)(1048576 - (CPU_INT32S)press_adc);                               /* Note 2 */
    press_comp = (press_comp - (CPU_INT32U)(var2 >> 12)) * 3125u;
    if (press_comp >= (1u << 30)) {
        press_comp = (press_comp / (CPU_INT32U)var1) << 1;
    } else {
        press_comp = (press_comp << 1) / (CPU_INT32U)var1;
    }

    var1 = ((CPU_INT32S)p_calib->par_p9 * (CPU_INT32S)(((press_comp >> 3) * (press_comp >> 3)) >> 13)) >> 12;
    var2 = ((CPU_INT32S)(press_comp >> 2) * (CPU_INT32S)p_calib->par_p8) >> 13;
    var3 = ((CPU_INT32S)(press_comp >> 8) * (CPU_INT32S)(press_comp >> 8) *
            (CPU_INT32S)(press_comp >> 8) * (CPU_INT32S)p_calib->par_p10) >> 17;

    return ((CPU_INT32U)((CPU_INT32S)press_comp +
                         ((var1 + var2 + var3 + ((CPU_INT32S)p_calib->par_p7 * 128)) >> 4)));     /* Note 1 */
}

/*
*********************************************************************************************************
*                                           env_calc_hum()
*
* Description : Humidity compensation (ch. 3.6, table 15).
*
* Argument(s) : p_calib     calibration values (par_h1..7).
*
*               hum_adc     raw humidity (16 bit).
*
*               temp_c100   compensated temperature in 0.01 degC.
*
* Return(s)   : Relative humidity in 0.001 %rH, limited to 0 ... 100 %rH.
*
* Caller(s)   : env_calc_data().
*
* Note(s)     : (1) Variant "* 1000" of the datasheet -> result in 0.001 %rH.
*               (2) "par_h1 << 4" -> "* 16", "par_h6 << 7" -> "* 128" (file Note 4).
*               (3) Limit added: values outside 0 ... 100 %rH are physically impossible (noise).
*********************************************************************************************************
*/

static CPU_INT32U env_calc_hum(const ENV_CALIB *p_calib, CPU_INT16U hum_adc, CPU_INT32S temp_c100)
{
    CPU_INT32S  var1;
    CPU_INT32S  var2;
    CPU_INT32S  var3;
    CPU_INT32S  var4;
    CPU_INT32S  var5;
    CPU_INT32S  var6;
    CPU_INT32S  hum_comp;


    var1 = (CPU_INT32S)hum_adc - ((CPU_INT32S)p_calib->par_h1 * 16) -                         /* Note 2 */
           (((temp_c100 * (CPU_INT32S)p_calib->par_h3) / 100) >> 1);
    var2 = ((CPU_INT32S)p_calib->par_h2 *
            (((temp_c100 * (CPU_INT32S)p_calib->par_h4) / 100) +
             (((temp_c100 * ((temp_c100 * (CPU_INT32S)p_calib->par_h5) / 100)) >> 6) / 100) +
             (CPU_INT32S)(1 << 14))) >> 10;
    var3 = var1 * var2;
    var4 = (((CPU_INT32S)p_calib->par_h6 * 128) +                                             /* Note 2 */
            ((temp_c100 * (CPU_INT32S)p_calib->par_h7) / 100)) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    hum_comp = (((var3 + var6) >> 10) * 1000) >> 12;                                          /* Note 1 */

    if (hum_comp < 0) {                                         /* Note 3                               */
        return (0u);
    }
    if ((CPU_INT32U)hum_comp > ENV_HUM_M1000_MAX) {
        return (ENV_HUM_M1000_MAX);
    }
    return ((CPU_INT32U)hum_comp);
}

/*
*********************************************************************************************************
*                                           env_calc_gas()
*
* Description : Gas resistance (ch. 3.7, table 17).
*
* Argument(s) : gas_adc     raw gas value (10 bit).
*
*               gas_range   gas ADC range (4 bit).
*
* Return(s)   : Gas resistance in Ohm.
*
* Caller(s)   : env_calc_data().
*
* Note(s)     : (1) "* 10000 ... * 100" instead of "* 1000000" as in the datasheet: no overflow.
*               (2) var2 >= 4096 - 3 * 512 = 2560, so the division is always valid.
*********************************************************************************************************
*/

static CPU_INT32U env_calc_gas(CPU_INT16U gas_adc, CPU_INT08U gas_range)
{
    CPU_INT32U  var1;
    CPU_INT32S  var2;


    var1 = 262144u >> gas_range;
    var2 = (CPU_INT32S)gas_adc - 512;
    var2 = var2 * 3;
    var2 = 4096 + var2;                                         /* Note 2                               */

    return (((10000u * var1) / (CPU_INT32U)var2) * 100u);      /* Note 1                               */
}

/* [] END OF FILE */
