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
#include <bsp_hr.h>
static CPU_INT16U spo2_get_chronological_index(
    const spo2_buffer *buffer,
    CPU_INT16U chronological_idx)
{
    CPU_INT16U oldest_idx;

    if (buffer->samples_count < SAMPLE_BUFFER_SIZE) {
        oldest_idx = 0;
    } else {
        oldest_idx = buffer->buffer_idx;
    }

    return (oldest_idx + chronological_idx) % SAMPLE_BUFFER_SIZE;
}

void calculate_heartrate(spo2_buffer *buffer, CPU_INT16U *heartrate){
    
    *heartrate = 0;

    if (buffer->samples_count < 100u) {
        return;
    }

    CPU_INT16U first_idx = spo2_get_chronological_index(buffer, 0);
    CPU_INT32S max_value = buffer->ir[first_idx];
    CPU_INT16U max_index = 0;
    
    CPU_INT16U peak_count = 0;
    CPU_INT16U last_peak = 0;
    CPU_INT32U avg_peak_diff = 0;
    
    for (CPU_INT16U i = 1; i < buffer->samples_count - 1; i++) {
        CPU_INT16U index = spo2_get_chronological_index(buffer, i);
        CPU_INT32S current = buffer->ir[index];
        
        if (current > max_value) {
            max_value = current;
            max_index = i;
        }
        
        if ((max_value - current) > PEAK_THRESHOLD) {
            if (peak_count == 0 || (max_index - last_peak) > MIN_PEAK_DISTANCE) {
                if (peak_count > 0) {
                    avg_peak_diff += (CPU_INT32U)(max_index - last_peak);
                }
                last_peak = max_index;
                peak_count++;
            }
            
            max_value = current;
            max_index = i;
        }
    }
    
    if (peak_count < 2) {
        return;
    }

    avg_peak_diff /= (peak_count - 1);
    
    if (avg_peak_diff == 0) {
        return;
    }
    
    CPU_INT32U calculated_hr = (60u * EFFECTIVE_SAMPLING_FREQ) / avg_peak_diff;
    if (calculated_hr < 40u || calculated_hr > 250u) {
        *heartrate = 0;
        return;
    }
    
    *heartrate = (60u * EFFECTIVE_SAMPLING_FREQ) / avg_peak_diff;
}

/* [] END OF FILE */
