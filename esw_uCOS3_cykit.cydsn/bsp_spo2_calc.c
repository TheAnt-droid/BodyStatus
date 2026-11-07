/*
*********************************************************************************************************
*                             OXIMETER 5 CLICK (MAX30102) - SPO2 CALCULATION
*
* Filename      : bsp_spo2_calc.c
*
* Description   : Ring buffer for the red/IR samples and SpO2 from their DC and AC parts. Used only by
*                 App_TaskSPO2CALC().
*********************************************************************************************************
*/
#include <bsp_spo2_calc.h>
/*
*********************************************************************************************************
*                                       spo2_get_chronological_index()
*
* Description : Converts a position in time order (0 = oldest sample) into the index of the ring buffer.
*
* Argument(s) : buffer      the ring buffer.
*
*               idx         position in time order.
*
* Return(s)   : Index into buffer->red[] / buffer->ir[].
*
* Caller(s)   : spo2_calc_dc(), spo2_calc_ac(), calculate_heartrate() (bsp_hr.c)
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
CPU_INT16U spo2_get_chronological_index(const spo2_buffer *buffer, CPU_INT16U idx)
{
    CPU_INT16U oldest_idx;
                                                                /* Buffer not full yet: the oldest      */
                                                                /* sample is still at position 0        */
    if (buffer->samples_count < SAMPLE_BUFFER_SIZE)
    {
        oldest_idx = 0;
    }
    else                                                        /* Buffer full: the oldest sample is    */
    {                                                           /* the next one to be overwritten       */
        oldest_idx = buffer->buffer_idx;
    }

    return (oldest_idx + idx) % SAMPLE_BUFFER_SIZE;
}
/*
*********************************************************************************************************
*                                       spo2_add_sample()
*
* Description : Writes the newest sample into the ring buffer. When it is full, the oldest one is overwritten.
*
* Argument(s) : red         red value.
*
*               ir          IR value.
*
*               buffer      the ring buffer.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
void spo2_add_sample(CPU_INT32U red, CPU_INT32U ir, spo2_buffer *buffer)
{
    
    buffer->red[buffer->buffer_idx] = red;
    buffer->ir[buffer->buffer_idx] = ir;

    buffer->buffer_idx++;
    if (buffer->buffer_idx >= SAMPLE_BUFFER_SIZE){              /* End of the array: start again at 0   */
                                                                /* (ring buffer)                        */
        buffer->buffer_idx = 0;
    }

    if (buffer->samples_count < SAMPLE_BUFFER_SIZE){            /* Count up to SAMPLE_BUFFER_SIZE, then */
                                                                /* the buffer is full                   */
        buffer->samples_count++;
    }
}
/*
*********************************************************************************************************
*                                       spo2_calc_dc()
*
* Description : DC part = average of the red and of the IR values in the buffer.
*
* Argument(s) : buffer      the ring buffer.
*
*               avg_red     receives the red average.
*
*               avg_ir      receives the IR average.
*
* Return(s)   : 1 if calculated, 0 if the buffer has no valid number of samples.
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
CPU_BOOLEAN spo2_calc_dc(spo2_buffer *buffer, CPU_INT32U *avg_red, CPU_INT32U *avg_ir)
{
    CPU_INT64U red_sum = 0;
    CPU_INT64U ir_sum = 0;
    if (buffer->samples_count == 0 || buffer->samples_count > SAMPLE_BUFFER_SIZE){
                                                                /* No samples or invalid count: no      */
                                                                /* calculation                          */
        return 0;
    }

    for (CPU_INT16U i = 0; i < buffer->samples_count; i++)      /* Sum of all samples in the buffer     */
    {
        CPU_INT16U index = spo2_get_chronological_index(buffer, i);

        red_sum += buffer->red[index];
        ir_sum += buffer->ir[index];
    }
    *avg_red = red_sum / buffer->samples_count;                 /* Average                              */
    *avg_ir = ir_sum / buffer->samples_count;

    return 1;
}
/*
*********************************************************************************************************
*                                       spo2_calc_ac()
*
* Description : AC part = maximum minus minimum (peak to peak) of the red and of the IR values in the buffer.
*
* Argument(s) : buffer      the ring buffer.
*
*               ac_red      receives the red AC part.
*
*               ac_ir       receives the IR AC part.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
void spo2_calc_ac(spo2_buffer *buffer, CPU_INT32U *ac_red, CPU_INT32U *ac_ir)
{

    CPU_INT32U max_red = 0;
    CPU_INT32U min_red = DEF_INT_32U_MAX_VAL;

    CPU_INT32U max_ir = 0;
    CPU_INT32U min_ir = DEF_INT_32U_MAX_VAL;

    for (CPU_INT16U i = 0; i < buffer->samples_count; i++)      /* Highest and lowest red and IR value  */
    {
        CPU_INT16U index = spo2_get_chronological_index(buffer, i);

        if (buffer->red[index] < min_red)
            min_red = buffer->red[index];

        if (buffer->red[index] > max_red)
            max_red = buffer->red[index];

        if (buffer->ir[index] < min_ir)
            min_ir = buffer->ir[index];

        if (buffer->ir[index] > max_ir)
            max_ir = buffer->ir[index];
    }
    *ac_red = max_red - min_red;                                /* AC = peak to peak                    */
    *ac_ir = max_ir - min_ir;
}
/*
*********************************************************************************************************
*                                       spo2_calc_ratio()
*
* Description : Ratio R = (AC_red / DC_red) / (AC_ir / DC_ir), calculated as
*               (AC_red * DC_ir) / (DC_red * AC_ir).
*
* Argument(s) : dc_red, dc_ir   DC parts from spo2_calc_dc().
*
*               ac_red, ac_ir   AC parts from spo2_calc_ac().
*
*               ratio           receives R, 0 if one of the products is 0 (no division by 0).
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
void spo2_calc_ratio(CPU_INT32U dc_red, CPU_INT32U dc_ir, CPU_INT32U ac_red, CPU_INT32U ac_ir, CPU_FP32 *ratio)
{
    CPU_FP32 top = ac_red * dc_ir;
    CPU_FP32 bot = dc_red * ac_ir;
    if (top != 0 && bot != 0)
    {
        *ratio = top / bot;
    }
    else
    {
        *ratio = 0;
    }
}
/*
*********************************************************************************************************
*                                       spo2_calc_percent()
*
* Description : SpO2 in percent from the ratio R (calibration curve, see the comment in the function).
*
* Argument(s) : ratio       R from spo2_calc_ratio().
*
*               spo2        receives SpO2 in percent.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Re-entrant: works only on the parameters.
*********************************************************************************************************
*/
void spo2_calc_percent(CPU_FP32 ratio, CPU_INT08U *spo2)
{
                                                                /* Calibration curve, predefined        */
                                                                /* formula from online                  */
    *spo2 = SPO2_CAL_A * ratio * ratio + SPO2_CAL_B * ratio + SPO2_CAL_C;
}

/* [] END OF FILE */
