/*
*********************************************************************************************************
*
*                                       MES1 Embedded Software (RTOS)
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) :

*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <cyapicallbacks.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_USER_IF_SIGN_ON 0u
#define APP_USER_IF_VER_TICK_RATE 1u
#define APP_USER_IF_CPU 2u
#define APP_USER_IF_CTXSW 3u
#define APP_USER_IF_STATE_MAX 4u

#define USE_BSP_TOGGLE 1u

#define SPO2_MEM_BLOCKS 32u

/*
*********************************************************************************************************
*                                            LOCAL STRUCS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

OS_SEM button_sem;

static OS_MEM SpO2Mem;
static spo2 SpO2MemPool[SPO2_MEM_BLOCKS];

static CPU_BOOLEAN spo2_init_success = DEF_TRUE;

static OS_TCB App_TaskStartTCB;
static CPU_STK App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB App_TaskLED_TCB;
static CPU_STK App_TaskLEDStk[APP_CFG_TASK_LED_STK_SIZE];

static OS_TCB App_TaskButton_TCB;
static CPU_STK App_TaskButtonStk[APP_CFG_TASK_BUTTON_STK_SIZE];

static OS_TCB App_TaskSPO2_TCB;
static CPU_STK App_TaskSPO2Stk[APP_CFG_TASK_SPO2_STK_SIZE];

static OS_TCB App_TaskSPO2CALC_TCB;
static CPU_STK App_TaskSPO2CALCStk[APP_CFG_TASK_SPO2CALC_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void App_TaskStart(void *p_arg);

static void App_TaskLED(void *p_arg);
static void App_TaskBUTTON(void *p_arg);
static void App_TaskSPO2(void *p_arg);
static void App_TaskSPO2CALC(void *p_arg);

static void App_TaskCreate(void);
static void App_ObjCreate(void);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : Startup Code.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR os_err;
    SPO2_ERROR spo2_err;

    BSP_PreInit(); /* Perform BSP pre-initialization.                      */

    CPU_Init(); /* Initialize the uC/CPU services                       */

    OSInit(&os_err); /* Init uC/OS-III.                                      */

    I2C_spo2_Start();
    Check_Sensor(SPO2_ADDRESS, SPO2_ID_REG, SPO2_ID_VAL, &spo2_err);
    if (spo2_err == SPO2_ERR_NONE)
    {
        spo2_config(SPO2_ADDRESS, &spo2_err);
    }
    if (spo2_err != SPO2_ERR_NONE)
    {
        I2C_spo2_Stop();
        spo2_init_success = DEF_FALSE;
    }

    OSTaskCreate((OS_TCB *)&App_TaskStartTCB, /* Create the start task                                */
                 (CPU_CHAR *)"Start",
                 (OS_TASK_PTR)App_TaskStart,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_START_PRIO,
                 (CPU_STK *)&App_TaskStartStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_START_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY)0u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);

    OSStart(&os_err); /* Start multitasking (i.e. give control to uC/OS-III).  */
}

/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static void App_TaskStart(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_PostInit(); /* Perform BSP post-initialization functions.       */

    BSP_CPU_TickInit(); /* Perfrom Tick Initialization                      */

#if (OS_CFG_STAT_TASK_EN > 0u)
    OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    App_TaskCreate(); /* Create application tasks.                        */

    App_ObjCreate(); /* Create kernel objects                            */

    while (DEF_TRUE)
    { /* Task body, always written as an infinite loop.   */
        OSTimeDlyHMSM(0, 0, 0, 100,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
    }
}

/*
*********************************************************************************************************
*                                          App_TaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void App_TaskCreate(void)
{
    /* declare and define function local variables */
    OS_ERR os_err;

    /* create LED task */
    OSTaskCreate((OS_TCB *)&App_TaskLED_TCB,
                 (CPU_CHAR *)"TaskLED",
                 (OS_TASK_PTR)App_TaskLED,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_LED_PRIO,
                 (CPU_STK *)&App_TaskLEDStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_LED_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_LED_STK_SIZE,
                 (OS_MSG_QTY)0u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);
    /* create Button task */
    OSTaskCreate((OS_TCB *)&App_TaskButton_TCB,
                 (CPU_CHAR *)"TaskBUT",
                 (OS_TASK_PTR)App_TaskBUTTON,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_BUTTON_PRIO,
                 (CPU_STK *)&App_TaskButtonStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_BUTTON_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_BUTTON_STK_SIZE,
                 (OS_MSG_QTY)0u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);
    if (spo2_init_success)
    {
        /* create SPO2 task */
        OSTaskCreate((OS_TCB *)&App_TaskSPO2_TCB,
                     (CPU_CHAR *)"TaskSPO2",
                     (OS_TASK_PTR)App_TaskSPO2,
                     (void *)0,
                     (OS_PRIO)APP_CFG_TASK_SPO2_PRIO,
                     (CPU_STK *)&App_TaskSPO2Stk[0],
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2_STK_SIZE_LIMIT,
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2_STK_SIZE,
                     (OS_MSG_QTY)0u,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&os_err);
        /* create UART task */
        OSTaskCreate((OS_TCB *)&App_TaskSPO2CALC_TCB,
                     (CPU_CHAR *)"TaskSPO2CALC",
                     (OS_TASK_PTR)App_TaskSPO2CALC,
                     (void *)0,
                     (OS_PRIO)APP_CFG_TASK_SPO2CALC_PRIO,
                     (CPU_STK *)&App_TaskSPO2CALCStk[0],
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2CALC_STK_SIZE_LIMIT,
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2CALC_STK_SIZE,
                     (OS_MSG_QTY)32u,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&os_err);
    }
}

/*
*********************************************************************************************************
*                                          App_ObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void App_ObjCreate(void)
{
    /* declare and define function local variables */
    OS_ERR os_err;

    /* create button semaphore to synchronize button press events */
    OSSemCreate(&button_sem, "SEM_BUT", 0, &os_err);

    OSMemCreate(&SpO2Mem,
                "Spo2MEM",
                SpO2MemPool,
                SPO2_MEM_BLOCKS,
                sizeof(spo2),
                &os_err);
}

/*
*********************************************************************************************************
*                                          App_TaskLED()
*
* Description : LED Task pends on a semaphore which is controlled by App_TaskButton(). If the button is
*               pressed the semaphore is constantly posted and this task can toggle the LED (P2_1, J1 Pin 2).
*               If the button is not pressed and the semaphore is not posted, this task waits to inifinity
*               until the button is pressed again. Via the define USE_BSP_TOGGLE both BSP functions can
*               be tested. If USE_BSP_TOGGLE is TRUE the BSP_LED_Toggle() function will be used, otherwise
*               BSP_LED_Off() and BSP_LED_On() will be used.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskLED()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

static void App_TaskLED(void *p_arg)
{
    /* declare and define task local variables */
    OS_ERR os_err;
    CPU_TS ts;

    /* prevent compiler warnings */
    (void)p_arg;

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        /* pend on semaphore - blocking */
        OSSemPend(&button_sem, 0, OS_OPT_PEND_BLOCKING, &ts, &os_err);
#if USE_BSP_TOGGLE
        /* toggle LED (P2_1, J1 Pin 2) */
        BSP_LED_Toggle(P2_1);
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 1, 0,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
#else
        /* turn LED (P2_1, J1 Pin 2) off */
        BSP_LED_Off(P2_1);
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 1, 0,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        /* turn LED (P2_1, J1 Pin 2) on */
        BSP_LED_On(P2_1);
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 1, 0,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
#endif
    }
}

/*
*********************************************************************************************************
*                                          App_TaskButton()
*
* Description : Button Task checks the state of a GND-connected button. On button press post
*               a semaphore. Otherwise, do not post a semaphore and check the button state again.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskButton()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

static void App_TaskBUTTON(void *p_arg)
{
    /* declare and define task local variables */
    OS_ERR os_err;
    CPU_INT08S button_status = 0;
    static CPU_BOOLEAN button_pressed = 0;

    /* prevent compiler warnings */
    (void)p_arg;

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        /* check is button is pressed */
        button_status = BSP_PB_StatusGet(P2_2);
        /* check the button status (pressed ... 0 or not pressed ... 1) */
        if (button_status)
        {
            /* if button not pressed reset button_pressed variable (Button State)*/
            button_pressed = 0;
        }
        else if (!button_status)
        {
            /* if button is pressed check button_pressed variable */
            if (!button_pressed)
            {
                /* if button_pressed (Button State) was not pressed before - post
                semaphore */
                OSSemPost(&button_sem, OS_OPT_POST_ALL, &os_err);
            }
            else
            {
                /* if button_pressed (Button State) was pressed before - post semaphore
                and set to 1 that only one pend is accepted*/
                OSSemSet(&button_sem, 1, &os_err);
            }
            /* in both cases set button_pressed (Button State) to 1 */
            button_pressed = 1;
        }
        else
        {
            /* error */
        }
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 0, 100,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
    }
}
/*
*********************************************************************************************************
*                                          App_TaskSPO2()
*
* Description :
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskSPO2()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
void uint32_to_str(CPU_INT32U val, char *p_buf)
{
    char temp[12];
    int i = 0, j = 0;

    if (val == 0)
    {
        p_buf[0] = '0';
        p_buf[1] = '\0';
        return;
    }

    while (val > 0)
    {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }

    // Reverse into output buffer
    while (i > 0)
    {
        p_buf[j++] = temp[--i];
    }
    p_buf[j] = '\0';
}
void float_to_str(float val, char *p_buf, int decimal_places)
{
    int j = 0;

    // 1. Handle negative numbers
    if (val < 0.0f)
    {
        p_buf[j++] = '-';
        val = -val;
    }

    // 2. Extract integer part
    long int_part = (long)val;

    // 3. Extract and round fractional part
    float remainder = val - (float)int_part;
    long mult = 1;
    for (int p = 0; p < decimal_places; p++)
    {
        mult *= 10;
    }
    long frac_part = (long)(remainder * (float)mult + 0.5f); // Round up

    // Handle carry-over if rounding rolls over the fractional part
    if (frac_part >= mult)
    {
        int_part += 1;
        frac_part = 0;
    }

    // 4. Convert integer part to string (into a temporary stack buffer)
    char temp[12];
    int i = 0;
    if (int_part == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        long temp_val = int_part;
        while (temp_val > 0)
        {
            temp[i++] = (temp_val % 10) + '0';
            temp_val /= 10;
        }
    }

    // Copy reversed integer part to output buffer
    while (i > 0)
    {
        p_buf[j++] = temp[--i];
    }

    // 5. Append decimal point and fractional part if requested
    if (decimal_places > 0)
    {
        p_buf[j++] = '.';

        char frac_temp[10];
        int frac_i = 0;

        for (int p = 0; p < decimal_places; p++)
        {
            frac_temp[frac_i++] = (frac_part % 10) + '0';
            frac_part /= 10;
        }

        // Copy reversed fractional part to output buffer
        while (frac_i > 0)
        {
            p_buf[j++] = frac_temp[--frac_i];
        }
    }

    // Null-terminate the final string
    p_buf[j] = '\0';
}

static void App_TaskSPO2(void *p_arg)
{
    /* prevent compiler warnings */
    (void)p_arg;

    OS_ERR os_err;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;

    static spo2 *sample;
    CPU_INT08U samples_available_num;
    CPU_BOOLEAN samples_available;

    while (DEF_TRUE)
    {
        samples_available = sample_available(SPO2_ADDRESS, &samples_available_num, &spo2_err);
        if (samples_available)
        {
            sample = (spo2 *)OSMemGet(&SpO2Mem, &os_err);
            if (os_err != OS_ERR_NONE)
            {
                uart_send_string((CPU_INT08U *)"os_mem_get failed\r\n");
                OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_HMSM_STRICT, &os_err);
                continue;
            }

            spo2_read_fifo_sample(SPO2_ADDRESS, &sample->red, &sample->ir, &spo2_err);
            if (spo2_err != SPO2_ERR_NONE)
            {
                OSMemPut(&SpO2Mem, (void *)sample, &os_err);
                OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_TIME_HMSM_STRICT, &os_err);
                continue;
            }

            if (sample->ir > FINGER_THRESHOLD && sample->red > FINGER_THRESHOLD)
            {
                OSTaskQPost(&App_TaskSPO2CALC_TCB, sample, sizeof(*sample), OS_OPT_POST_FIFO, &os_err);

                if (os_err != OS_ERR_NONE)
                {
                    OSMemPut(&SpO2Mem, (void *)sample, &os_err);
                }
            }
            else
            {
                OSMemPut(&SpO2Mem, (void *)sample, &os_err);
            }
        }

        OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

/*
*********************************************************************************************************
*                                          App_TaskSPO2CALC()
*
* Description :
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskSPO2()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

static void App_TaskSPO2CALC(void *p_arg)
{
    /* prevent compiler warnings */
    (void)p_arg;

    OS_ERR os_err;
    OS_MSG_SIZE p_msg;
    CPU_TS ts;

    static spo2 *sample;
    static oxi_results results;
    CPU_INT16U results_index = 0;

    spo2_buffer buffer;
    buffer.buffer_idx = 0;
    buffer.samples_count = 0;

    CPU_INT32U red_dc;
    CPU_INT32U ir_dc;

    CPU_INT32U red_ac;
    CPU_INT32U ir_ac;

    float ratio;
    CPU_INT08U spo2;
    CPU_INT16U heartrate;

    while (DEF_TRUE)
    {

        sample = OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &p_msg, &ts, &os_err);

        if (os_err == OS_ERR_NONE && sample != DEF_NULL)
        {
            spo2_add_sample(sample->red, sample->ir, &buffer);

            OSMemPut(&SpO2Mem, (void *)sample, &os_err);
            if (os_err != OS_ERR_NONE)
            {
                uart_send_string((CPU_INT08U *)"ERROR: OSMemPut failed\r\n");
            }

            if (buffer.samples_count > 0)
            {
                if (spo2_calc_dc(&buffer, &red_dc, &ir_dc))
                {
                    spo2_calc_ac(&buffer, &red_ac, &ir_ac);

                    spo2_calc_ratio(red_dc, ir_dc, red_ac, ir_ac, &ratio);

                    spo2_calc_percent(ratio, &spo2);
                    calculate_heartrate(&buffer, &heartrate);

                    results.spo2 += spo2;
                    results.hr += heartrate;
                    results_index++;

                    if (results_index >= EFFECTIVE_SAMPLING_FREQ)
                    {                                  // For output every second
                        results.spo2 /= results_index; // Average of 1 second of samples
                        results.hr /= results_index;   // Average of 1 second of samples

                        // Send to LED Task- Does not exist yet, so UART output instead :)
                        char hr_str[20];
                        uint32_to_str(results.hr, hr_str);
                        char spo2_str[20];
                        uint32_to_str(results.spo2, spo2_str);
                        uart_send_string((CPU_INT08U *)"HR: ");
                        uart_send_string((CPU_INT08U *)hr_str);
                        uart_send_string((CPU_INT08U *)" SpO2: ");
                        uart_send_string((CPU_INT08U *)spo2_str);
                        uart_send_string((CPU_INT08U *)"\r\n");
                        results.spo2 = 0;
                        results.hr = 0;
                        results_index = 0;
                    }
                }
            }
        }

        OSTimeDlyHMSM(0, 0, 0, 1,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
    }
}

/* END OF FILE */