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
/*
*********************************************************************************************************
*                                       spo2_get_chronological_index()
*
* Description : Gets the chronological index of a ring buffer
*
* Argument(s) : *buffer ... the ring buffer
                idx ... non chronological index
*
* Return(s)   : spo2_error
*
* Caller(s)   : Local.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
static CPU_INT16U spo2_get_chronological_index(const spo2_buffer *buffer,CPU_INT16U idx){
    CPU_INT16U oldest_idx;
/*If sample count is smaller than buffer size, the oldest index is still on position 0*/
    if (buffer->samples_count < SAMPLE_BUFFER_SIZE) {
        oldest_idx = 0;
    } else {
        /*If sample count is bigger than buffer size, the oldest index is no longer position 0*/
        oldest_idx = buffer->buffer_idx;
    }

    return (oldest_idx + idx) % SAMPLE_BUFFER_SIZE;
}
/*
*********************************************************************************************************
*                                       calculate_heartrate()
*
* Description : Calculates the heartrate based on ir value. Uses peak detection to find the sample difference between peaks.
*
* Argument(s) : *buffer ... buffer containing ir values
                *heartrate ... pointer to heartrate value
                
*
* Return(s)   : none
*
* Caller(s)   : Application.
*
* Note(s)     : Not re-entrant.
*********************************************************************************************************
*/
void calculate_heartrate(spo2_buffer *buffer, CPU_INT16U *heartrate){
    
    *heartrate = 0;

    if (buffer->samples_count < 100u) {
        /*Only calculate if at least 100 samples have been collected*/
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
            /*If new max was found, save it and its index*/
            max_value = current;
            max_index = i;
        }
        
        if ((max_value - current) > PEAK_THRESHOLD) {
            /*To avoid peak detection to find noise peaks, a certain threshold must be passed to save the peak*/
            if (peak_count == 0 || (max_index - last_peak) > MIN_PEAK_DISTANCE) {
                /*Peaks must be a certain distance apart*/
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
    
    if (peak_count < NUM_PEAKS_FOR_HR) {
        /*As soon as enough peaks have been collected, return*/
        return;
    }
    /*Calculate the average samples between peaks*/
    avg_peak_diff /= (peak_count - 1);
    
    if (avg_peak_diff == 0) {
        return;
    }
    /*Calculate the Heartrate from collected avg peak difference*/
    CPU_INT32U calculated_hr = (60u * EFFECTIVE_SAMPLING_FREQ) / avg_peak_diff;
    if (calculated_hr < LOW_HR_BORDER || calculated_hr > HIGH_HR_BORDER) {
        /*Plausability check*/
        *heartrate = 0;
        return;
    }
    /*Return calculated heartrate if everything goes well :)*/
    *heartrate = (60u * EFFECTIVE_SAMPLING_FREQ) / avg_peak_diff;
}

/* [] END OF FILE */
