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
#include <bsp_spo2_calc.h>
static CPU_INT16U spo2_get_chronological_index(
    const spo2_buffer *buffer,
    CPU_INT16U chronological_idx)
{
    CPU_INT16U oldest_idx;

    if (buffer->samples_count < SAMPLE_BUFFER_SIZE)
    {
        oldest_idx = 0;
    }
    else
    {
        oldest_idx = buffer->buffer_idx;
    }

    return (oldest_idx + chronological_idx) % SAMPLE_BUFFER_SIZE;
}

void spo2_add_sample(CPU_INT32U red, CPU_INT32U ir, spo2_buffer *buffer)
{

    buffer->red[buffer->buffer_idx] = red;
    buffer->ir[buffer->buffer_idx] = ir;

    buffer->buffer_idx++;
    if (buffer->buffer_idx >= SAMPLE_BUFFER_SIZE)
    {
        buffer->buffer_idx = 0;
    }

    if (buffer->samples_count < SAMPLE_BUFFER_SIZE)
    {
        buffer->samples_count++;
    }
}

CPU_BOOLEAN spo2_calc_dc(spo2_buffer *buffer, CPU_INT32U *avg_red, CPU_INT32U *avg_ir)
{
    CPU_INT64U red_sum = 0;
    CPU_INT64U ir_sum = 0;
    if (buffer->samples_count == 0 || buffer->samples_count > SAMPLE_BUFFER_SIZE)
    {
        return 0;
    }

    for (CPU_INT16U i = 0; i < buffer->samples_count; i++)
    {

        CPU_INT16U index = spo2_get_chronological_index(buffer, i);

        red_sum += buffer->red[index];
        ir_sum += buffer->ir[index];
    }

    *avg_red = red_sum / buffer->samples_count;
    *avg_ir = ir_sum / buffer->samples_count;

    return 1;
}

void spo2_calc_ac(spo2_buffer *buffer, CPU_INT32U *ac_red, CPU_INT32U *ac_ir)
{

    CPU_INT32U max_red = 0;
    CPU_INT32U min_red = 0xFFFFFFFFu;

    CPU_INT32U max_ir = 0;
    CPU_INT32U min_ir = 0xFFFFFFFFu;

    for (CPU_INT16U i = 0; i < buffer->samples_count; i++)
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

    *ac_red = max_red - min_red;
    *ac_ir = max_ir - min_ir;
}

void spo2_calc_ratio(CPU_INT32U dc_red, CPU_INT32U dc_ir, CPU_INT32U ac_red, CPU_INT32U ac_ir, float *ratio)
{
    float top = ac_red * dc_ir;
    float bot = dc_red * ac_ir;
    if (top != 0 && bot != 0)
    {
        *ratio = top / bot;
    }
    else
    {
        *ratio = 0;
    }
}

void spo2_calc_percent(float ratio, CPU_INT08U *spo2)
{

    *spo2 = -45.060f * ratio * ratio + 30.354f * ratio + 94.845f;
}

/* [] END OF FILE */
