/*
*********************************************************************************************************
*                                OXIMETER 5 CLICK (MAX30102) - HEART RATE
*
* Filename      : bsp_hr.c
*
* Description   : Heart rate from the IR samples in the ring buffer of App_TaskSPO2CALC(): finds the
*                 pulse peaks and converts the average distance between them into beats per minute.
*********************************************************************************************************
*/
#include <bsp_hr.h>
#include <bsp_spo2_calc.h>                                      /* spo2_get_chronological_index()       */
/*
*********************************************************************************************************
*                                       calculate_heartrate()
*
* Description : Calculates the heart rate from the IR values: finds the peaks in the buffer and converts
*               the average distance between them into beats per minute.
*
* Argument(s) : buffer      ring buffer with the IR values.
*
*               heartrate   receives the heart rate in bpm, 0 if there is no valid value.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) A peak counts when IR falls PEAK_THRESHOLD below the maximum since the last peak.
*                   Peaks closer than MIN_PEAK_DISTANCE samples are ignored (noise).
*               (2) heart rate = 60 s * EFFECTIVE_SAMPLING_FREQ / average samples between two peaks.
*                   Values outside LOW_HR_BORDER..HIGH_HR_BORDER are discarded.
*********************************************************************************************************
*/
void calculate_heartrate(spo2_buffer *buffer, CPU_INT16U *heartrate){
    
    *heartrate = 0;

    if (buffer->samples_count < HR_MIN_SAMPLES) {               /* Only calculate with at least         */
                                                                /* HR_MIN_SAMPLES samples               */
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
        
        if (current > max_value) {                              /* New maximum: remember value and      */
                                                                /* position                             */
            max_value = current;
            max_index = i;
        }
        
        if ((max_value - current) > PEAK_THRESHOLD) {           /* IR has fallen far enough below the   */
                                                                /* maximum: the maximum was a peak      */
            if (peak_count == 0 || (max_index - last_peak) > MIN_PEAK_DISTANCE) {
                                                                /* Only count peaks with enough         */
                                                                /* distance to the last one             */
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
    
    if (peak_count < NUM_PEAKS_FOR_HR) {                        /* Not enough peaks for a reliable      */
                                                                /* value: heart rate stays 0            */
        return;
    }
    avg_peak_diff /= (peak_count - 1);                          /* Average number of samples between    */
                                                                /* two peaks                            */
    
    if (avg_peak_diff == 0) {
        return;
    }
                                                                /* Samples between peaks -> beats per   */
                                                                /* minute                               */
    CPU_INT32U calculated_hr = (HR_S_PER_MIN * EFFECTIVE_SAMPLING_FREQ) / avg_peak_diff;
    if (calculated_hr < LOW_HR_BORDER || calculated_hr > HIGH_HR_BORDER) {
                                                                /* Plausibility check                   */
        *heartrate = 0;
        return;
    }
    *heartrate = calculated_hr;                                 /* Valid heart rate                     */
}

/* [] END OF FILE */
