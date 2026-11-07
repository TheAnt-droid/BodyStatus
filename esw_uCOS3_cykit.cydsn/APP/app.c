/*
*********************************************************************************************************
*
*                                       MES1 Embedded Software (RTOS)
*
*                                         BODY STATUS ANALYSER
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : Anthea (SpO2, button), Dennis (LED), Giulia (environment)
*
* Description   : Application of the Body Status Analyser on the CY8CKIT-059 with the Click Shield:
*                   - Button task       : switches the measurement on/off (APP_FLAG_MEAS_ACTIVE)
*                   - SpO2 tasks        : Oximeter 5 Click (I2C, slot 2) -> SpO2 and heart rate
*                   - LED task          : heartbeat (red) and SpO2 (blue) on the RGB LED
*                   - LED_ENV task      : room climate (green) on the RGB LED
*                   - Environment task  : Environment 3 Click (BME688, SPI, slot 1) -> room climate
*                 The tasks are coupled by the event flag group App_Flags (see Note 1 below the
*                 flag defines) and by task message queues.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <cyapicallbacks.h>
#include <os_cfg_app.h>                                         /* OS_CFG_TICK_RATE_HZ for ms -> ticks  */
#include <lib_str.h>                                            /* Str_FmtNbr_Int32S() for debug output */

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

                                                                /* ---- General ------------------------ */
#define  APP_MS_PER_S                            1000u
#define  APP_MS_TO_TICKS(ms)                     (((ms) * OS_CFG_TICK_RATE_HZ) / APP_MS_PER_S)
#define  APP_PEND_FOREVER                  (OS_TICK)0u          /* Timeout 0 = wait forever             */
#define  APP_PERCENT                              100u
#define  APP_START_DLY_MS                         100u          /* Start task: sleep time per loop      */

                                                                /* ---- Button task -------------------- */
#define  APP_BTN_POLL_DLY_MS                      100u          /* Check the button every 100 ms        */
#define  APP_BTN_PRESSED                 ((CPU_INT08S)0)        /* BSP_PB_StatusGet(): low-active       */
#define  APP_BTN_RELEASED                ((CPU_INT08S)1)

                                                                /* ---- SpO2 calculation --------------- */
#define  APP_SPO2_GAP_MS                          500u          /* No sample for 0.5 s: start again     */
                                                                /* (normal: one sample every 20 ms)     */
#define  APP_SPO2CALC_Q_SIZE                       32u          /* Samples waiting for SpO2CALC         */
#define  APP_SPO2_OFF_DLY_MS                      100u          /* Sensor off: check again after 100 ms */
#define  APP_SPO2_NO_SMP_DLY_MS                    10u          /* FIFO empty: check again after 10 ms  */
#define  APP_SPO2_ERR_DLY_MS                        5u          /* After an error: try again after 5 ms */
#define  APP_SPO2_READ_DLY_MS                      20u          /* One sample every 20 ms (50 Hz)       */

                                                                /* ---- LED tasks ---------------------- */
#define  APP_LED_Q_SIZE                             4u          /* Results waiting for the LED task     */
#define  APP_MS_PER_MIN                         60000u          /* Beat length = APP_MS_PER_MIN / hr    */
#define  APP_LED_FLASH_MS                         100u          /* Red on-time per heartbeat            */
#define  APP_LED_FLASH_MIN_MS                      10u          /* Shortest red on-time                 */
#define  APP_LED_FLASH_DIV                          2u          /* High heart rate: on-time = half beat */
#define  APP_LED_NO_DATA_MS                      2500u          /* No result for 2.5 s: red/blue off    */
#define  APP_LED_ENV_DLY_MS                        20u          /* Green LED: read flags every 20 ms    */
#define  APP_LED_ENV_BLINK_CNT                     25u          /* 25 x 20 ms = 0.5 s per blink phase   */

                                                                /* ---- Debug output via UART ---------- */
#define  APP_ENV_DEBUG_EN                  DEF_ENABLED          /* Env values on UART; DISABLE for the  */
                                                                /* final version                        */
#define  APP_SPO2_DEBUG_EN                 DEF_ENABLED          /* SpO2/HR on UART; DISABLE for the     */
                                                                /* final version                        */
#define  APP_DEBUG_EN    ((APP_ENV_DEBUG_EN == DEF_ENABLED) || (APP_SPO2_DEBUG_EN == DEF_ENABLED))
#define  APP_UART_LINE_SIZE                        96u          /* One debug line incl. NUL             */
#define  APP_NBR_DIG               (DEF_INT_32S_NBR_DIG_MAX + 1u) /* Max. digits incl. sign             */
#define  APP_NBR_STR_SIZE                (APP_NBR_DIG + 2u)     /* + sign + NUL (lib_str.c Note 5)      */

                                                                /* ---- Event flags of App_Flags ------- */
                                                                /* Interface between the tasks, Note 1  */
#define  APP_FLAG_MEAS_ACTIVE              (OS_FLAGS)0x01u      /* Set/cleared by button: measuring on  */
#define  APP_FLAG_ENV_VALID                (OS_FLAGS)0x02u      /* Set by env task: values are valid    */
#define  APP_FLAG_ENV_OK                   (OS_FLAGS)0x04u      /* Set by env task: room climate is OK  */
#define  APP_FLAG_ENV_ALL                  (APP_FLAG_ENV_VALID | APP_FLAG_ENV_OK)
#define  APP_FLAGS_INIT                    (OS_FLAGS)0x00u      /* All flags cleared after reset        */

/* Note 1: Event flags instead of a semaphore: several tasks only READ the state. A pend without
*          OS_OPT_PEND_FLAG_CONSUME does not change the flags, so no task takes the event from another.
*/

                                                                /* ---- SpO2 memory partitions --------- */
#define  SPO2_MEM_BLOCKS                           32u          /* Blocks for raw samples  (SpO2Mem)    */
#define  OXI_MEM_BLOCKS                            32u          /* Blocks for results      (OxiResMem)  */

                                                                /* ---- Environment task --------------- */
#define  APP_ENV_PERIOD_MS                       3000u          /* 3 s = Bosch "low power mode" for     */
                                                                /* interactive use (datasheet ch. 4.2)  */
#define  APP_ENV_STARTUP_DLY_MS                     3u          /* Sensor start-up 2 ms + 1 tick margin */
#define  APP_ENV_MEAS_DLY_MS                      100u          /* First wait = heater-on time          */
#define  APP_ENV_POLL_DLY_MS                       10u          /* Then check every 10 ms for new data  */
#define  APP_ENV_POLL_MAX                          20u          /* Give up after 20 checks (200 ms)     */

                                                                /* ---- Acceptable indoor range -------- */
#define  APP_ENV_TEMP_MIN_C100          ((CPU_INT32S)1900)      /* 19 degC    (AStV par. 28)            */
#define  APP_ENV_TEMP_MAX_C100          ((CPU_INT32S)2500)      /* 25 degC    (AStV par. 28)            */
#define  APP_ENV_HUM_MIN_M1000                  40000u          /* 40 %rH     (AStV par. 28)            */
#define  APP_ENV_HUM_MAX_M1000                  70000u          /* 70 %rH     (AStV par. 28)            */
#define  APP_ENV_PRESS_MIN_PA                   95000u          /*  950 hPa   (plausibility only)       */
#define  APP_ENV_PRESS_MAX_PA                  105000u          /* 1050 hPa   (plausibility only)       */
#define  APP_ENV_GAS_WARMUP_MS                  90000u          /* Gas run-in, see App_EnvEvaluate() N.4*/
#define  APP_ENV_GAS_WARMUP_CNT         ((APP_ENV_GAS_WARMUP_MS + APP_ENV_PERIOD_MS - 1u) / APP_ENV_PERIOD_MS)
                                                                /* = 30 measurements, rounded up        */
#define  APP_ENV_GAS_MIN_PCT                       50u          /* Bad air: R < 50 % of the baseline    */

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

                                                                /* ---- Kernel objects ----------------- */
static  OS_FLAG_GRP   App_Flags;                                /* Shared state of all tasks, Note 1    */
static  OS_MUTEX      App_UartMutex;                            /* UART is used by several tasks        */

static  OS_MEM        SpO2Mem;                                  /* Raw samples  SpO2 -> SpO2CALC        */
static  spo2          SpO2MemPool[SPO2_MEM_BLOCKS];

static  OS_MEM        OxiResMem;                                /* Results      SpO2CALC -> LED         */
static  oxi_results   OxiResMemPool[OXI_MEM_BLOCKS];

                                                                /* ---- State -------------------------- */
static  CPU_BOOLEAN   spo2_init_success = DEF_TRUE;             /* Set in main(): oximeter found?       */

                                                                /* ---- Task TCBs and stacks ----------- */
                                                                /* (in order of priority, app_cfg.h)    */
static  OS_TCB        App_TaskStartTCB;
static  CPU_STK       App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB        App_TaskButton_TCB;
static  CPU_STK       App_TaskButtonStk[APP_CFG_TASK_BUTTON_STK_SIZE];

static  OS_TCB        App_TaskSPO2_TCB;
static  CPU_STK       App_TaskSPO2Stk[APP_CFG_TASK_SPO2_STK_SIZE];

static  OS_TCB        App_TaskSPO2CALC_TCB;
static  CPU_STK       App_TaskSPO2CALCStk[APP_CFG_TASK_SPO2CALC_STK_SIZE];

static  OS_TCB        App_TaskLED_TCB;
static  CPU_STK       App_TaskLEDStk[APP_CFG_TASK_LED_STK_SIZE];

static  OS_TCB        App_TaskLEDENV_TCB;
static  CPU_STK       App_TaskLEDEStk[APP_CFG_TASK_LEDE_STK_SIZE];

static  OS_TCB        App_TaskEnvTCB;
static  CPU_STK       App_TaskEnvStk[APP_CFG_TASK_ENV_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void         App_TaskStart    (void *p_arg);
static  void         App_TaskCreate   (void);
static  void         App_ObjCreate    (void);

static  void         App_TaskBUTTON   (void *p_arg);
static  void         App_TaskSPO2     (void *p_arg);
static  void         App_TaskSPO2CALC (void *p_arg);
static  void         App_TaskLED      (void *p_arg);
static  void         App_TaskLEDE_ENV (void *p_arg);
static  void         OximeterData     (oxi_results *oximeter, OS_ERR *p_err, CPU_INT32U *hr_ptr,
                                       CPU_INT32U *spo2_ptr);

static  void         App_TaskEnv      (void *p_arg);
static  CPU_BOOLEAN  App_EnvInit      (ENV_CALIB *p_calib);
static  CPU_BOOLEAN  App_EnvMeasure   (const ENV_CALIB *p_calib, ENV_DATA *p_data);
static  CPU_BOOLEAN  App_EnvEvaluate  (const ENV_DATA *p_data, CPU_INT32U *p_gas_base, CPU_INT08U *p_gas_cnt);

static  void         App_UartPrint    (CPU_CHAR *p_str);
#if (APP_ENV_DEBUG_EN == DEF_ENABLED)
static  void         App_EnvPrint     (const ENV_DATA *p_data, CPU_INT32U gas_base, CPU_INT08U gas_cnt,
                                       CPU_BOOLEAN env_ok);
#endif
#if (APP_SPO2_DEBUG_EN == DEF_ENABLED)
static  void         App_Spo2Print    (const oxi_results *p_results);
#endif
#if (APP_DEBUG_EN)
static  void         App_StrCatNbr    (CPU_CHAR *p_line, const CPU_CHAR *p_label, CPU_INT32S nbr);
#endif

static  void         App_ErrCheck     (OS_ERR err);


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code. Initialises the CPU and the OS, checks the
*               oximeter and creates the start task.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : Startup Code.
*
* Note(s)     : (1) The oximeter (I2C) is checked here, before the tasks exist. If it does not answer,
*                   spo2_init_success is cleared and App_TaskCreate() does not create the SpO2 tasks.
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR os_err;
    SPO2_ERROR spo2_err;

    BSP_PreInit();                                              /* Perform BSP pre-initialization.      */

    CPU_Init();                                                 /* Initialize the uC/CPU services       */

    OSInit(&os_err);                                            /* Init uC/OS-III.                      */
    App_ErrCheck(os_err);

    I2C_spo2_Start();                                           /* Check + configure oximeter (Note 1)  */
    Check_Sensor(SPO2_ADDRESS, SPO2_ID_REG, SPO2_ID_VAL, &spo2_err);
    if (spo2_err == SPO2_ERR_NONE)
    {
        spo2_config(SPO2_ADDRESS, &spo2_err);
    }
    if (spo2_err != SPO2_ERR_NONE)
    {
        I2C_spo2_Stop();
        spo2_init_success = DEF_FALSE;
    }else{
        spo2_sleep();
    }

    OSTaskCreate((OS_TCB *)&App_TaskStartTCB,                   /* Create the start task                */
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
    App_ErrCheck(os_err);

    OSStart(&os_err);                                           /* Start multitasking (never returns)   */
}

/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : Start task. Initialises the BSP and the tick (only possible once multitasking has
*               started), creates the kernel objects and the application tasks and then only sleeps.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) Kernel objects are created BEFORE the tasks that use them.
*               (2) After reset all flags are cleared: nothing is measured until the button is pressed.
*********************************************************************************************************
*/

static void App_TaskStart(void *p_arg)
{
    OS_ERR err;

    (void)p_arg;

    BSP_PostInit();                                             /* Perform BSP post-initialization.     */

    BSP_CPU_TickInit();                                         /* Perform tick initialization          */

#if (OS_CFG_STAT_TASK_EN > 0u)
    OSStatTaskCPUUsageInit(&err);
    App_ErrCheck(err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    App_ObjCreate();                                            /* Kernel objects first (Note 1)        */
    App_TaskCreate();                                           /* Create application tasks (Note 2).   */

    while (DEF_TRUE)                                            /* Nothing more to do: only sleep       */
    {
        OSTimeDlyHMSM(0u, 0u, 0u, APP_START_DLY_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);
        App_ErrCheck(err);
    }
}

/*
*********************************************************************************************************
*                                          App_TaskCreate()
*
* Description : Create application tasks (in order of priority, see app_cfg.h).
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : App_TaskStart()
*
* Note(s)     : (1) The SpO2 tasks are only created if the oximeter answered in main().
*               (2) The SpO2CALC task gets a task message queue for the samples of App_TaskSPO2().
*               (3) The LED task gets a task message queue for the results of App_TaskSPO2CALC().
*********************************************************************************************************
*/

static void App_TaskCreate(void)
{
    OS_ERR os_err;

    /* create button task */
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
    App_ErrCheck(os_err);

    if (spo2_init_success)                                      /* Note 1                               */
    {
        /* create SpO2 task */
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
        App_ErrCheck(os_err);

        /* create SpO2CALC task (Note 2) */
        OSTaskCreate((OS_TCB *)&App_TaskSPO2CALC_TCB,
                     (CPU_CHAR *)"TaskSPO2CALC",
                     (OS_TASK_PTR)App_TaskSPO2CALC,
                     (void *)0,
                     (OS_PRIO)APP_CFG_TASK_SPO2CALC_PRIO,
                     (CPU_STK *)&App_TaskSPO2CALCStk[0],
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2CALC_STK_SIZE_LIMIT,
                     (CPU_STK_SIZE)APP_CFG_TASK_SPO2CALC_STK_SIZE,
                     (OS_MSG_QTY)APP_SPO2CALC_Q_SIZE,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&os_err);
        App_ErrCheck(os_err);
    }

    /* create LED task (Note 3) */
    OSTaskCreate((OS_TCB *)&App_TaskLED_TCB,
                 (CPU_CHAR *)"TaskLED",
                 (OS_TASK_PTR)App_TaskLED,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_LED_PRIO,
                 (CPU_STK *)&App_TaskLEDStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_LED_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_LED_STK_SIZE,
                 (OS_MSG_QTY)APP_LED_Q_SIZE,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);
    App_ErrCheck(os_err);

    /* create LED_ENV task */
    OSTaskCreate((OS_TCB *)&App_TaskLEDENV_TCB,
                 (CPU_CHAR *)"TaskLED_ENV",
                 (OS_TASK_PTR)App_TaskLEDE_ENV,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_LEDE_PRIO,
                 (CPU_STK *)&App_TaskLEDEStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_LEDE_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_LEDE_STK_SIZE,
                 (OS_MSG_QTY)0u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);
    App_ErrCheck(os_err);

    /* create environment task */
    OSTaskCreate((OS_TCB *)&App_TaskEnvTCB,
                 (CPU_CHAR *)"TaskENV",
                 (OS_TASK_PTR)App_TaskEnv,
                 (void *)0,
                 (OS_PRIO)APP_CFG_TASK_ENV_PRIO,
                 (CPU_STK *)&App_TaskEnvStk[0],
                 (CPU_STK_SIZE)APP_CFG_TASK_ENV_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE)APP_CFG_TASK_ENV_STK_SIZE,
                 (OS_MSG_QTY)0u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&os_err);
    App_ErrCheck(os_err);
}

/*
*********************************************************************************************************
*                                          App_ObjCreate()
*
* Description : Create application kernel objects (event flags, memory partitions).
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : App_TaskStart()
*
* Note(s)     : (1) Memory partitions instead of malloc(): the message content must stay valid after
*                   the sending task continues ("Thou shalt", lecture).
*               (2) Mutex instead of semaphore: priority inheritance prevents priority inversion.
*********************************************************************************************************
*/

static void App_ObjCreate(void)
{
    OS_ERR os_err;

    /* event flag group shared by button, SpO2, LED and environment task */
    OSFlagCreate(&App_Flags, "FLG_APP", APP_FLAGS_INIT, &os_err);
    App_ErrCheck(os_err);

    /* mutex for the UART, which is shared by several tasks (Note 2) */
    OSMutexCreate(&App_UartMutex, "MTX_UART", &os_err);
    App_ErrCheck(os_err);

    /* memory partition for the raw samples SpO2 -> SpO2CALC (Note 1) */
    OSMemCreate(&SpO2Mem,
                "Spo2MEM",
                SpO2MemPool,
                SPO2_MEM_BLOCKS,
                sizeof(spo2),
                &os_err);
    App_ErrCheck(os_err);

    /* memory partition for the results SpO2CALC -> LED (Note 1) */
    OSMemCreate(&OxiResMem,
                "OxiMEM",
                OxiResMemPool,
                OXI_MEM_BLOCKS,
                sizeof(oxi_results),
                &os_err);
    App_ErrCheck(os_err);
}

/*
*********************************************************************************************************
*                                          App_TaskBUTTON()
*
* Description : Button task. Checks the low-active button (P2_2) every APP_BTN_POLL_DLY_MS. Each NEW press
*               (edge from released to pressed) switches the measurement on/off by setting or clearing
*               APP_FLAG_MEAS_ACTIVE in App_Flags.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskBUTTON()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) Only this task changes APP_FLAG_MEAS_ACTIVE, so it can keep the current state in a
*                   local variable (meas_active) and does not have to read the flag group.
*               (2) Polling every 100 ms also debounces the button: bouncing is shorter than 100 ms.
*               (3) Holding the button produces only one event (edge detection with button_pressed).
*********************************************************************************************************
*/

static void App_TaskBUTTON(void *p_arg)
{
    /* declare and define task local variables */
    OS_ERR os_err;
    CPU_INT08S button_status;
    CPU_BOOLEAN button_pressed = DEF_FALSE;
    CPU_BOOLEAN meas_active = DEF_FALSE;                        /* After reset: not measuring (Note 1)  */

    /* prevent compiler warnings */
    (void)p_arg;

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        /* read the button (low-active) */
        button_status = BSP_PB_StatusGet(P2_2);
        if (button_status == APP_BTN_RELEASED)
        {
            /* button released: reset the button state */
            button_pressed = DEF_FALSE;
        }
        else if (button_status == APP_BTN_PRESSED)
        {
            /* button pressed: only a NEW press is an event (Note 3) */
            if (button_pressed == DEF_FALSE)
            {
                if (meas_active == DEF_FALSE)
                {
                    /* measurement on: all measuring tasks start */
                    OSFlagPost(&App_Flags, APP_FLAG_MEAS_ACTIVE, OS_OPT_POST_FLAG_SET, &os_err);
                    meas_active = DEF_TRUE;
                }
                else
                {
                    /* measurement off: all measuring tasks stop */
                    OSFlagPost(&App_Flags, APP_FLAG_MEAS_ACTIVE, OS_OPT_POST_FLAG_CLR, &os_err);
                    meas_active = DEF_FALSE;
                }
                App_ErrCheck(os_err);
            }
            /* remember: button is pressed */
            button_pressed = DEF_TRUE;
        }
        else
        {
            /* error of BSP_PB_StatusGet(): ignore this check, try again next time */
        }
        /* release the CPU until the next check (Note 2) */
        OSTimeDlyHMSM(0u, 0u, 0u, APP_BTN_POLL_DLY_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        App_ErrCheck(os_err);
    }
}

/*
*********************************************************************************************************
*                                          App_TaskSPO2()
*
* Description : SpO2 task. While the measurement is active, it reads the red/IR samples from the FIFO of
*               the Oximeter 5 Click (I2C, slot 2). Samples with a finger on the sensor are sent in a
*               memory block (SpO2Mem) via the task queue to App_TaskSPO2CALC(), all others are discarded.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskSPO2()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) Inactive: blocking pend until APP_FLAG_MEAS_ACTIVE is SET, the CPU is released.
*               (2) Active: non-blocking pend whether APP_FLAG_MEAS_ACTIVE is CLEARED (CLR_ALL). OS_ERR_NONE
*                   = measurement off; OS_ERR_PEND_WOULD_BLOCK = still on, continue measuring.
*                   No CONSUME: the flag is also read by the environment task.
*********************************************************************************************************
*/

static void App_TaskSPO2(void *p_arg)
{
    /* prevent compiler warnings */
    (void)p_arg;

    /* task variables */
    OS_ERR os_err;
    SPO2_ERROR spo2_err = SPO2_ERR_NONE;

    CPU_TS ts;

    CPU_BOOLEAN active=DEF_FALSE;

    static spo2 *sample;
    CPU_INT08U samples_available_num;
    CPU_BOOLEAN samples_available;

    /* start of the endless loop */
    while (DEF_TRUE)
    {

        if(!active) {

            /* inactive: wait until the measurement is switched on, nothing else (Note 1) */
            OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,
                       (OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_BLOCKING), &ts, &os_err);
            App_ErrCheck(os_err);

            if(os_err == OS_ERR_NONE) {
                /* measurement on: wake up the sensor and set to active */
                spo2_wake();
                active = DEF_TRUE;
            }

        } else {

            /* active: check (without blocking) whether the measurement was switched off (Note 2) */
            OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,
                       (OS_OPT_PEND_FLAG_CLR_ALL | OS_OPT_PEND_NON_BLOCKING), &ts, &os_err);
            if (os_err != OS_ERR_PEND_WOULD_BLOCK) {
                App_ErrCheck(os_err);                           /* anything else than NONE is an error  */
            }
            if(os_err == OS_ERR_NONE) {
                /* measurement off: send the sensor to sleep and set to inactive */
                spo2_sleep();
                active = DEF_FALSE;
                /* skip the measurement code: delay and start again from the top */
                OSTimeDlyHMSM(0u, 0u, 0u, APP_SPO2_OFF_DLY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
                App_ErrCheck(os_err);
                continue;
            }
            /* are samples available? how many? */
            samples_available = sample_available(SPO2_ADDRESS, &samples_available_num, &spo2_err);

            if(!samples_available){
                /* no samples available: delay and start again from the top */
                OSTimeDlyHMSM(0u, 0u, 0u, APP_SPO2_NO_SMP_DLY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
                App_ErrCheck(os_err);
                continue;
            }
            /* reserve a memory block for the sample */
            sample = (spo2 *)OSMemGet(&SpO2Mem, &os_err);

            if (os_err != OS_ERR_NONE){
                /* error handling */
                App_UartPrint("os_mem_get failed\r\n");
                OSTimeDlyHMSM(0u, 0u, 0u, APP_SPO2_ERR_DLY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
                App_ErrCheck(os_err);
                continue;
            }
            /* read one sample from the FIFO into the memory block */
            spo2_read_fifo_sample(SPO2_ADDRESS, &sample->red, &sample->ir, &spo2_err);
            if (spo2_err != SPO2_ERR_NONE){
                /* error handling: give the block back */
                OSMemPut(&SpO2Mem, (void *)sample, &os_err);
                App_ErrCheck(os_err);
                OSTimeDlyHMSM(0u, 0u, 0u, APP_SPO2_ERR_DLY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
                App_ErrCheck(os_err);
                continue;
            }

            /* finger on the sensor? */
            if (sample->ir > FINGER_THRESHOLD && sample->red > FINGER_THRESHOLD)
            {
                /* only send data with a finger, otherwise it would be ambient light */
                OSTaskQPost(&App_TaskSPO2CALC_TCB, sample, sizeof(*sample), OS_OPT_POST_FIFO, &os_err);

                if (os_err != OS_ERR_NONE){
                    /* error handling: give the block back */
                    OSMemPut(&SpO2Mem, (void *)sample, &os_err);
                    App_ErrCheck(os_err);
                }
            }
            else
            {
                /* no finger: data not needed */
                OSMemPut(&SpO2Mem, (void *)sample, &os_err);
                App_ErrCheck(os_err);
            }

            /* release the CPU */
            OSTimeDlyHMSM(0u, 0u, 0u, APP_SPO2_READ_DLY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
            App_ErrCheck(os_err);
        }
    }
}

/*
*********************************************************************************************************
*                                          App_TaskSPO2CALC()
*
* Description : SpO2 calculation task. Receives the samples of App_TaskSPO2() via its task queue, collects
*               them in a buffer and calculates SpO2 and heart rate. The results are averaged over one
*               second (EFFECTIVE_SAMPLING_FREQ samples) and sent in a memory block (OxiResMem) via the
*               task queue to App_TaskLED().
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskSPO2CALC()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) The values of one second are summed up in local variables. Only when the second is
*                   complete, a memory block is taken from OxiResMem, filled with the averages and sent.
*                   The block then belongs to the LED task (it gives it back with OSMemPut()); this task
*                   must not touch it any more.
*               (2) If the LED task's queue is full (or does not exist yet), the post fails: the block is
*                   given back here, so no block is lost.
*               (3) No sample for APP_SPO2_GAP_MS (measurement switched off or finger removed): buffer and
*                   sums are cleared, so a new measurement never mixes old and new samples.
*********************************************************************************************************
*/

static void App_TaskSPO2CALC(void *p_arg)
{
    /* prevent compiler warnings */
    (void)p_arg;

    /* task variables */
    OS_ERR os_err;
    OS_MSG_SIZE p_msg;
    CPU_TS ts;

    static spo2 *sample;
    oxi_results *results;
    CPU_INT32U spo2_sum = 0u;                                   /* sums of one second (Note 1)          */
    CPU_INT32U hr_sum = 0u;
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

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        /* wait for a message, at most APP_SPO2_GAP_MS */
        sample = OSTaskQPend(APP_MS_TO_TICKS(APP_SPO2_GAP_MS), OS_OPT_PEND_BLOCKING, &p_msg, &ts, &os_err);
        if (os_err == OS_ERR_TIMEOUT){
            /* no sample: start again with an empty buffer (Note 3) */
            buffer.buffer_idx = 0;
            buffer.samples_count = 0;
            spo2_sum = 0u;
            hr_sum = 0u;
            results_index = 0;
        } else {
            App_ErrCheck(os_err);
        }

        if (os_err == OS_ERR_NONE && sample != DEF_NULL){
            /* message received: add the sample to the buffer */
            spo2_add_sample(sample->red, sample->ir, &buffer);

            /* release the memory block: after copying into the buffer it is no longer needed */
            OSMemPut(&SpO2Mem, (void *)sample, &os_err);
            if (os_err != OS_ERR_NONE){
                /* error handling */
                App_UartPrint("ERROR: OSMemPut failed\r\n");
            }

            if (buffer.samples_count >=SAMPLE_BUFFER_SIZE)
            {  
                /* samples in the buffer: start the calculation */
                if (spo2_calc_dc(&buffer, &red_dc, &ir_dc)){
                    /* DC calculation successful: AC calculation */
                    spo2_calc_ac(&buffer, &red_ac, &ir_ac);
                    /* ratio calculation */
                    spo2_calc_ratio(red_dc, ir_dc, red_ac, ir_ac, &ratio);
                    /* SpO2 in percent */
                    spo2_calc_percent(ratio, &spo2);
                    /* heart rate */
                    calculate_heartrate(&buffer, &heartrate);

                    /* add the new values to the sums (Note 1) */
                    spo2_sum += spo2;
                    hr_sum += heartrate;
                    results_index++;

                    if (results_index >= EFFECTIVE_SAMPLING_FREQ){ /* output once per second */
                        /* take a memory block for the result (Note 1) */
                        results = (oxi_results *)OSMemGet(&OxiResMem, &os_err);
                        if (os_err == OS_ERR_NONE){
                            /* average over 1 second of samples */
                            results->spo2 = spo2_sum / results_index;
                            results->hr = hr_sum / results_index;
#if (APP_SPO2_DEBUG_EN == DEF_ENABLED)
                            App_Spo2Print(results);             /* debug output, before the block is sent */
#endif

                            /* send to the LED task */
                            OSTaskQPost(&App_TaskLED_TCB, results, sizeof(*results), OS_OPT_POST_FIFO, &os_err);
                            if(os_err!=OS_ERR_NONE){
                                /* not sent: give the block back (Note 2) */
                                OSMemPut(&OxiResMem,(void *)results,&os_err);
                                App_ErrCheck(os_err);
                            }
                        }

                        /* start the next second */
                        spo2_sum = 0u;
                        hr_sum = 0u;
                        results_index = 0;
                    }
                }
            }
        }
    }
}

/*
*********************************************************************************************************
*                                           OximeterData()
*
* Description : Takes over one result of App_TaskSPO2CALC(): copies SpO2 and heart rate, gives the memory
*               block back to OxiResMem and sets the brightness of the blue LED to the SpO2 value.
*
* Argument(s) : oximeter    pointer to the received result block (DEF_NULL after a timeout).
*
*               p_err       result of the OSTaskQPend() that delivered the block.
*
*               hr_ptr      receives the heart rate in beats per minute.
*
*               spo2_ptr    receives SpO2 in percent.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskLED()
*
* Note(s)     : (1) After a timeout there is no block. Nothing is changed, the LED task keeps the last
*                   values.
*               (2) The block comes from OxiResMem and has to go back there. Given back to another
*                   partition, it would be missing in OxiResMem, which would be empty after
*                   OXI_MEM_BLOCKS results.
*               (3) SpO2 is limited to 100 % before it is used for the brightness.
*********************************************************************************************************
*/

static void OximeterData(oxi_results *oximeter, OS_ERR *p_err, CPU_INT32U *hr_ptr, CPU_INT32U *spo2_ptr)
{
    OS_ERR      mem_err;
    CPU_INT32U  buffer_spo2;

    if ((*p_err == OS_ERR_NONE) && (oximeter != DEF_NULL))      /* Note 1                               */
    {
        *spo2_ptr = oximeter->spo2;
        *hr_ptr   = oximeter->hr;

        OSMemPut(&OxiResMem, (void *)oximeter, &mem_err);       /* Note 2                               */
        App_ErrCheck(mem_err);

        buffer_spo2 = (*spo2_ptr > APP_PERCENT) ? APP_PERCENT : *spo2_ptr;   /* Note 3                  */
        LED_spo2(&buffer_spo2);
    }
}

/*
*********************************************************************************************************
*                                           App_TaskLED()
*
* Description : LED task for the oximeter. The red LED flashes once per heartbeat, the brightness of the
*               blue LED shows SpO2. The results come from App_TaskSPO2CALC() via the task message queue,
*               one per second.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskLED()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) No heart rate (hr == 0, also after reset): red off, wait without timeout for the first
*                   result.
*               (2) One beat lasts APP_MS_PER_MIN / hr ms. The red LED is on for flash ms and off for the
*                   rest of the beat. Both times are pend timeouts: the task releases the CPU and takes over
*                   a new result while it waits. A timeout is the normal case here.
*               (3) If a beat is not longer than flash, flash becomes half a beat (at least
*                   APP_LED_FLASH_MIN_MS) and stays like that.
*               (4) No result for APP_LED_NO_DATA_MS (finger removed or measurement off): hr = 0 and blue
*                   off, then back to Note 1.
*********************************************************************************************************
*/

static void App_TaskLED(void *p_arg)
{
    /* declare and define task local variables */
    OS_ERR        os_err;
    CPU_TS        ts;
    OS_MSG_SIZE   msg_size;
    oxi_results  *oximeter;
    CPU_INT32U    spo2       = 0u;
    CPU_INT32U    hr         = 0u;
    CPU_INT32U    flash      = APP_LED_FLASH_MS;
    CPU_INT32U    no_data_ms = 0u;
    CPU_INT32U    beat_duration;
    CPU_INT32U    red_led_rest;
    CPU_INT32U    spo2_off;

    /* prevent compiler warnings */
    (void)p_arg;

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        if (hr == 0u)
        {
            /* no heart rate: red off, wait for the first result (Note 1) */
            LED_off("r");
            oximeter = (oxi_results *)OSTaskQPend(APP_PEND_FOREVER, OS_OPT_PEND_BLOCKING, &msg_size, &ts, &os_err);
            App_ErrCheck(os_err);
            OximeterData(oximeter, &os_err, &hr, &spo2);
            continue;                                           /* check hr again                       */
        }

        /* length of one beat and of the red flash (Note 2, 3) */
        beat_duration = APP_MS_PER_MIN / hr;
        if (beat_duration <= flash)
        {
            flash = beat_duration / APP_LED_FLASH_DIV;
            if (flash < APP_LED_FLASH_MIN_MS)
            {
                flash = APP_LED_FLASH_MIN_MS;
            }
        }
        red_led_rest = beat_duration - flash;

        /* red on for flash ms, a new result can come in meanwhile */
        LED_on("r");
        oximeter = (oxi_results *)OSTaskQPend((OS_TICK)APP_MS_TO_TICKS(flash), OS_OPT_PEND_BLOCKING,
                                              &msg_size, &ts, &os_err);
        if (os_err == OS_ERR_TIMEOUT)
        {
            no_data_ms += flash;                                /* normal case, no new result (Note 2)  */
        }
        else
        {
            App_ErrCheck(os_err);
            no_data_ms = 0u;
        }
        OximeterData(oximeter, &os_err, &hr, &spo2);
        if (hr == 0u)
        {
            continue;                                           /* result without heart rate: Note 1    */
        }

        /* red off for the rest of the beat, a new result can come in meanwhile */
        LED_off("r");
        oximeter = (oxi_results *)OSTaskQPend((OS_TICK)APP_MS_TO_TICKS(red_led_rest), OS_OPT_PEND_BLOCKING,
                                              &msg_size, &ts, &os_err);
        if (os_err == OS_ERR_TIMEOUT)
        {
            no_data_ms += red_led_rest;
        }
        else
        {
            App_ErrCheck(os_err);
            no_data_ms = 0u;
        }
        OximeterData(oximeter, &os_err, &hr, &spo2);

        /* no result for too long: heart rate unknown (Note 4) */
        if (no_data_ms >= APP_LED_NO_DATA_MS)
        {
            spo2_off   = 0u;
            hr         = 0u;
            no_data_ms = 0u;
            LED_spo2(&spo2_off);                                /* blue off, red goes off at the top    */
        }
    }
}

/*
*********************************************************************************************************
*                                         App_TaskLEDE_ENV()
*
* Description : LED task for the room climate. Reads App_Flags every APP_LED_ENV_DLY_MS and shows the
*               result of App_TaskEnv() on the green LED:
*                   - measurement off or no valid values  : off
*                   - valid values, room climate OK       : on
*                   - valid values, room climate not OK   : blinking, APP_LED_ENV_BLINK_CNT loops per phase
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskLEDE_ENV()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) One non-blocking pend with SET_ANY returns all requested flags that are set, so one
*                   call reads the whole state. Without CONSUME the flags stay unchanged for the other
*                   tasks. OS_ERR_PEND_WOULD_BLOCK only means that none of them is set.
*               (2) The task has to run regularly for the blinking, so it sleeps with a delay instead of
*                   blocking on the flags.
*********************************************************************************************************
*/

static void App_TaskLEDE_ENV(void *p_arg)
{
    /* declare and define task local variables */
    OS_ERR        os_err;
    CPU_TS        ts;
    OS_FLAGS      env_flag_status;
    CPU_INT32U    blink_counter   = 0u;
    CPU_BOOLEAN   led_blink_state = DEF_FALSE;

    /* prevent compiler warnings */
    (void)p_arg;

    /* start of the endless loop */
    while (DEF_TRUE)
    {
        /* read the current state of the flags (Note 1) */
        env_flag_status = OSFlagPend(&App_Flags, (APP_FLAG_MEAS_ACTIVE | APP_FLAG_ENV_ALL), APP_PEND_FOREVER,
                                     (OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_NON_BLOCKING), &ts, &os_err);
        if (os_err == OS_ERR_PEND_WOULD_BLOCK)
        {
            env_flag_status = APP_FLAGS_INIT;                   /* none of the flags is set             */
        }
        else
        {
            App_ErrCheck(os_err);
        }

        if (((env_flag_status & APP_FLAG_MEAS_ACTIVE) == 0u) ||
            ((env_flag_status & APP_FLAG_ENV_VALID)   == 0u))
        {
            /* not measuring or no valid values: off */
            LED_off("g");
            blink_counter   = 0u;
            led_blink_state = DEF_FALSE;
        }
        else if ((env_flag_status & APP_FLAG_ENV_OK) != 0u)
        {
            /* valid and OK: on */
            LED_on("g");
        }
        else
        {
            /* valid but not OK: toggle after APP_LED_ENV_BLINK_CNT loops */
            blink_counter++;
            if (blink_counter >= APP_LED_ENV_BLINK_CNT)
            {
                blink_counter   = 0u;
                led_blink_state = !led_blink_state;
            }
            if (led_blink_state)
            {
                LED_on("g");
            }
            else
            {
                LED_off("g");
            }
        }

        /* release the CPU until the next check (Note 2) */
        OSTimeDlyHMSM(0u, 0u, 0u, APP_LED_ENV_DLY_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        App_ErrCheck(os_err);
    }
}

/*
*********************************************************************************************************
*                                          App_TaskEnv()
*
* Description : Environment task. While APP_FLAG_MEAS_ACTIVE is set, it measures temperature, pressure,
*               humidity and gas resistance every APP_ENV_PERIOD_MS with the BME688 (Environment 3 Click,
*               SPI, slot 1), checks them against the acceptable indoor range and reports the result
*               with the event flags APP_FLAG_ENV_VALID and APP_FLAG_ENV_OK.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskEnv()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Caller(s)   : This is a task.
*
* Note(s)     : (1) Calibration, values and gas baseline are local: only this task needs them.
*               (2) Without CONSUME, so the flag stays set for the other tasks. Measuring off: clear the
*                   ENV flags (no old values shown), restart the gas run-in (the heater only runs while
*                   measuring), then wait blocking. -> one endless loop, CPU released.
*               (3) Sensor errors can happen at runtime (e.g. bad contact): no exception, ENV flags
*                   cleared, sensor initialised again next cycle. OS errors -> App_ErrCheck().
*               (4) Bad values: first clear OK, then set VALID -> never "VALID + OK" for bad values.
*               (5) Only for debugging (APP_ENV_DEBUG_EN): one line per measurement on the UART.
*********************************************************************************************************
*/

static void App_TaskEnv(void *p_arg)
{
    OS_ERR       err;
    CPU_TS       ts;
    ENV_CALIB    calib;
    ENV_DATA     data;
    CPU_INT32U   gas_base;
    CPU_INT08U   gas_cnt;
    CPU_BOOLEAN  env_ready;
    CPU_BOOLEAN  env_ok;


    (void)p_arg;

    gas_base  = 0u;                                             /* No gas baseline yet                  */
    gas_cnt   = 0u;
    env_ready = App_EnvInit(&calib);                            /* Check sensor, calibration, config    */

    while (DEF_TRUE)
    {
                                                                /* Measuring on? (Note 2)               */
        OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,
                   (OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_NON_BLOCKING), &ts, &err);
        if (err == OS_ERR_PEND_WOULD_BLOCK)                     /* No: clear old result ...             */
        {
            OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_CLR, &err);
            App_ErrCheck(err);
            gas_cnt = 0u;                                       /* Gas run-in starts again (Note 2)     */
            OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,  /* ... and wait until it is on */
                       (OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_BLOCKING), &ts, &err);
        }
        App_ErrCheck(err);

        if (env_ready == DEF_FAIL)                              /* Sensor not ready: try again (Note 3) */
        {
            env_ready = App_EnvInit(&calib);
        }

        if ((env_ready == DEF_OK) && (App_EnvMeasure(&calib, &data) == DEF_OK))
        {
            env_ok = App_EnvEvaluate(&data, &gas_base, &gas_cnt);
#if (APP_ENV_DEBUG_EN == DEF_ENABLED)
            App_EnvPrint(&data, gas_base, gas_cnt, env_ok);     /* Debug output (Note 5)                */
#endif
            if (env_ok == DEF_OK)
            {
                OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_SET, &err);    /* Valid + OK      */
            }
            else
            {
                OSFlagPost(&App_Flags, APP_FLAG_ENV_OK, OS_OPT_POST_FLAG_CLR, &err);     /* Not OK (Note 4) */
                App_ErrCheck(err);
                OSFlagPost(&App_Flags, APP_FLAG_ENV_VALID, OS_OPT_POST_FLAG_SET, &err);  /* but valid       */
            }
            App_ErrCheck(err);
        }
        else                                                    /* Measurement failed (Note 3)          */
        {
            env_ready = DEF_FAIL;
            OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_CLR, &err);
            App_ErrCheck(err);
        }

        OSTimeDly(APP_MS_TO_TICKS(APP_ENV_PERIOD_MS), OS_OPT_TIME_DLY, &err);   /* Release CPU until next */
        App_ErrCheck(err);                                                      /* measurement            */
    }
}

/*
*********************************************************************************************************
*                                          App_EnvInit()
*
* Description : Checks whether the BME688 answers on the SPI bus, reads its calibration values and
*               writes the measurement settings.
*
* Argument(s) : p_calib   Calibration values (output).
*
* Return(s)   : DEF_OK    sensor ready.
*               DEF_FAIL  sensor does not answer correctly (wrong slot, bad contact, ...).
*
* Caller(s)   : App_TaskEnv()
*
* Note(s)     : (1) Each step only runs if the previous one was successful.
*               (2) Start-up time 2 ms (datasheet table 1). An OS delay can be up to 1 tick shorter than
*                   requested, so 3 ms guarantee at least 2 ms.
*********************************************************************************************************
*/

static CPU_BOOLEAN App_EnvInit(ENV_CALIB *p_calib)
{
    OS_ERR       os_err;
    ENV_ERR      env_err;
    CPU_BOOLEAN  result;


    OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_STARTUP_DLY_MS,           /* Wait for sensor start-up (Note 2)    */
                  OS_OPT_TIME_HMSM_STRICT,
                  &os_err);
    App_ErrCheck(os_err);

    env_err = env_check_id();                                   /* Read chip id + variant id            */
    if (env_err == ENV_ERR_NONE)
    {
        env_err = env_read_calib(p_calib);                      /* Read calibration values              */
    }
    if (env_err == ENV_ERR_NONE)
    {
        env_err = env_config(p_calib);                          /* Oversampling, filter, gas heater     */
    }

    if (env_err == ENV_ERR_NONE)
    {
        result = DEF_OK;
    }
    else
    {
        result = DEF_FAIL;
    }
    return (result);
}

/*
*********************************************************************************************************
*                                          App_EnvMeasure()
*
* Description : Performs one measurement in forced mode and converts the raw values into degC, Pa, %rH
*               and Ohm.
*
* Argument(s) : p_calib   Calibration values from App_EnvInit().
*               p_data    Converted values (output).
*
* Return(s)   : DEF_OK    p_data contains new values.
*               DEF_FAIL  SPI error or no new data within the time limit.
*
* Caller(s)   : App_TaskEnv()
*
* Note(s)     : (1) The datasheet gives no total measurement time: first wait the heater-on time, then
*                   check new_data every few ms, at most APP_ENV_POLL_MAX times. Every wait is an OS
*                   delay, so the CPU is released.
*********************************************************************************************************
*/

static CPU_BOOLEAN App_EnvMeasure(const ENV_CALIB *p_calib, ENV_DATA *p_data)
{
    OS_ERR       os_err;
    ENV_ERR      env_err;
    ENV_RAW      raw;
    CPU_INT08U   poll;
    CPU_BOOLEAN  result;


    result       = DEF_FAIL;
    raw.new_data = DEF_NO;

    env_err = env_start_meas();                                 /* Start one measurement (forced mode)  */
    if (env_err == ENV_ERR_NONE)
    {
        OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_MEAS_DLY_MS,          /* Wait at least the heater-on time     */
                      OS_OPT_TIME_HMSM_STRICT,                  /* (Note 1)                             */
                      &os_err);
        App_ErrCheck(os_err);

        for (poll = 0u;                                         /* Check for new data, limited (Note 1) */
             (poll < APP_ENV_POLL_MAX) && (raw.new_data == DEF_NO) && (env_err == ENV_ERR_NONE);
             poll++)
        {
            env_err = env_read_raw(&raw);
            if ((env_err == ENV_ERR_NONE) && (raw.new_data == DEF_NO))  /* Not finished: release CPU */
            {
                OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_POLL_DLY_MS,
                              OS_OPT_TIME_HMSM_STRICT,
                              &os_err);
                App_ErrCheck(os_err);
            }
        }

        if ((env_err == ENV_ERR_NONE) && (raw.new_data == DEF_YES))
        {
            result = env_calc_data(p_calib, &raw, p_data);      /* Convert into degC, Pa, %rH, Ohm      */
        }
    }
    return (result);
}

/*
*********************************************************************************************************
*                                          App_EnvEvaluate()
*
* Description : Checks whether all measured values are in the acceptable indoor range.
*
* Argument(s) : p_data       Converted values from App_EnvMeasure().
*               p_gas_base   Gas baseline in Ohm (input/output, kept by the task).
*               p_gas_cnt    Number of valid gas values since start (input/output, kept by the task).
*
* Return(s)   : DEF_OK    all values acceptable.
*               DEF_FAIL  at least one value outside the range.
*
* Caller(s)   : App_TaskEnv()
*
* Note(s)     : (1) Temperature and humidity: Austrian workplace regulation (AStV par. 28). Pressure: only
*                   a plausibility check. The sensor temperature is typically slightly above the ambient
*                   temperature (self-heating, datasheet table 10).
*               (2) Gas: no absolute limit exists, more VOC = lower resistance. Baseline = highest value
*                   so far (cleanest air); the air is bad if R < APP_ENV_GAS_MIN_PCT % of the baseline.
*                   Same idea as the background calibration of Bosch BSEC (datasheet ch. 3.3): the
*                   reference is the sensor's own history. Only valid values (gas_ok) are used.
*               (3) Divide first, then multiply: no overflow of CPU_INT32U.
*               (4) Run-in: right after the start the gas resistance is not yet stable. The datasheet
*                   gives no time for this, so APP_ENV_GAS_WARMUP_MS is a conservative choice. Values of
*                   this phase are ignored completely - also for the baseline, because the baseline only
*                   grows: one too high value would stay the reference forever.
*********************************************************************************************************
*/

static CPU_BOOLEAN App_EnvEvaluate(const ENV_DATA *p_data, CPU_INT32U *p_gas_base, CPU_INT08U *p_gas_cnt)
{
    CPU_BOOLEAN  result;


    result = DEF_OK;

    if ((p_data->temp_c100 < APP_ENV_TEMP_MIN_C100) ||          /* Temperature (Note 1)                 */
        (p_data->temp_c100 > APP_ENV_TEMP_MAX_C100))
    {
        result = DEF_FAIL;
    }
    if ((p_data->hum_m1000 < APP_ENV_HUM_MIN_M1000) ||          /* Humidity (Note 1)                    */
        (p_data->hum_m1000 > APP_ENV_HUM_MAX_M1000))
    {
        result = DEF_FAIL;
    }
    if ((p_data->press_pa < APP_ENV_PRESS_MIN_PA) ||            /* Pressure (Note 1)                    */
        (p_data->press_pa > APP_ENV_PRESS_MAX_PA))
    {
        result = DEF_FAIL;
    }

    if (p_data->gas_ok == DEF_YES)                              /* Gas (Note 2)                         */
    {
        if (*p_gas_cnt < APP_ENV_GAS_WARMUP_CNT)                /* Run-in: ignore the value (Note 4)    */
        {
            (*p_gas_cnt)++;
        }
        else
        {
            if (p_data->gas_ohm > *p_gas_base)                  /* Cleaner air than before: new base    */
            {
                *p_gas_base = p_data->gas_ohm;
            }
            if (p_data->gas_ohm < ((*p_gas_base / APP_PERCENT) * APP_ENV_GAS_MIN_PCT))    /* Note 3     */
            {
                result = DEF_FAIL;
            }
        }
    }

    return (result);
}

/*
*********************************************************************************************************
*                                          App_UartPrint()
*
* Description : Sends a string via UART. Protected by a mutex, because several tasks use the UART.
*
* Argument(s) : p_str   NUL-terminated string.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2(), App_TaskSPO2CALC(), App_EnvPrint(), App_Spo2Print().
*
* Note(s)     : (1) The mutex keeps one line together: another task can only send when this line is done.
*               (2) uart_send_string() is not re-entrant (bsp_uart.c), the mutex makes the access safe.
*********************************************************************************************************
*/

static void App_UartPrint(CPU_CHAR *p_str)
{
    OS_ERR  err;
    CPU_TS  ts;


    OSMutexPend(&App_UartMutex, APP_PEND_FOREVER, OS_OPT_PEND_BLOCKING, &ts, &err);   /* Note 1       */
    App_ErrCheck(err);

    uart_send_string((CPU_INT08U *)p_str);                      /* Note 2                               */

    OSMutexPost(&App_UartMutex, OS_OPT_POST_NONE, &err);
    App_ErrCheck(err);
}

#if (APP_ENV_DEBUG_EN == DEF_ENABLED)
/*
*********************************************************************************************************
*                                          App_EnvPrint()
*
* Description : Debug output of one environment measurement, e.g.
*               "ENV T=2234 P=101025 H=45621 G=25300 B=30000 N=30 OK=1"
*               T in 0.01 degC, P in Pa, H in 0.001 %rH, G = gas in Ohm, B = gas baseline in Ohm,
*               N = gas values so far (run-in until APP_ENV_GAS_WARMUP_CNT), OK = 1 if all values are OK.
*
* Argument(s) : p_data     converted values.
*               gas_base   gas baseline in Ohm.
*               gas_cnt    number of valid gas values so far.
*               env_ok     result of App_EnvEvaluate().
*
* Return(s)   : none
*
* Caller(s)   : App_TaskEnv()
*
* Note(s)     : (1) The line is built completely first and then sent with ONE App_UartPrint(), so lines
*                   of different tasks cannot mix.
*               (2) Integers only (no printf / float): Str_FmtNbr_Int32S() of uC/LIB.
*********************************************************************************************************
*/

static void App_EnvPrint(const ENV_DATA *p_data, CPU_INT32U gas_base, CPU_INT08U gas_cnt, CPU_BOOLEAN env_ok)
{
    CPU_CHAR  line[APP_UART_LINE_SIZE];


    line[0] = ASCII_CHAR_NULL;                                  /* Empty line, then add the values      */
    App_StrCatNbr(line, "ENV T=", p_data->temp_c100);
    App_StrCatNbr(line, " P=",    (CPU_INT32S)p_data->press_pa);
    App_StrCatNbr(line, " H=",    (CPU_INT32S)p_data->hum_m1000);
    App_StrCatNbr(line, " G=",    (CPU_INT32S)p_data->gas_ohm);
    App_StrCatNbr(line, " B=",    (CPU_INT32S)gas_base);
    App_StrCatNbr(line, " N=",    (CPU_INT32S)gas_cnt);
    App_StrCatNbr(line, " OK=",   (env_ok == DEF_OK) ? 1 : 0);
    (void)Str_Cat_N(line, "\r\n", APP_UART_LINE_SIZE - 1u - Str_Len(line));

    App_UartPrint(line);                                        /* Note 1                               */
}
#endif

#if (APP_SPO2_DEBUG_EN == DEF_ENABLED)
/*
*********************************************************************************************************
*                                          App_Spo2Print()
*
* Description : Debug output of one SpO2 result (average over one second), e.g. "SPO2 HR=72 SPO2=97"
*               HR = heart rate in beats per minute, SPO2 = oxygen saturation in %.
*
* Argument(s) : p_results   averaged result of App_TaskSPO2CALC().
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSPO2CALC()
*
* Note(s)     : (1) Called BEFORE the block is posted to the LED task: afterwards the block belongs to the
*                   LED task and must not be read any more.
*               (2) Only with a finger on the sensor (otherwise no samples, no results).
*********************************************************************************************************
*/

static void App_Spo2Print(const oxi_results *p_results)
{
    CPU_CHAR  line[APP_UART_LINE_SIZE];


    line[0] = ASCII_CHAR_NULL;
    App_StrCatNbr(line, "SPO2 HR=", (CPU_INT32S)p_results->hr);
    App_StrCatNbr(line, " SPO2=",   (CPU_INT32S)p_results->spo2);
    (void)Str_Cat_N(line, "\r\n", APP_UART_LINE_SIZE - 1u - Str_Len(line));

    App_UartPrint(line);                                        /* one line at once, mutex protected    */
}
#endif

#if (APP_DEBUG_EN)
/*
*********************************************************************************************************
*                                          App_StrCatNbr()
*
* Description : Appends a label and a number to a line, e.g. " P=" and 101025.
*
* Argument(s) : p_line    line (NUL-terminated, APP_UART_LINE_SIZE characters).
*               p_label   text before the number.
*               nbr       number.
*
* Return(s)   : none
*
* Caller(s)   : App_EnvPrint(), App_Spo2Print()
*
* Note(s)     : (1) Str_Cat_N() never writes more than the free space of the line.
*********************************************************************************************************
*/

static void App_StrCatNbr(CPU_CHAR *p_line, const CPU_CHAR *p_label, CPU_INT32S nbr)
{
    CPU_CHAR  nbr_str[APP_NBR_STR_SIZE];


    (void)Str_FmtNbr_Int32S(nbr, APP_NBR_DIG, DEF_NBR_BASE_DEC, ASCII_CHAR_NULL,  /* No leading chars  */
                            DEF_NO, DEF_YES, nbr_str);
    (void)Str_Cat_N(p_line, p_label, APP_UART_LINE_SIZE - 1u - Str_Len(p_line));  /* Note 1           */
    (void)Str_Cat_N(p_line, nbr_str, APP_UART_LINE_SIZE - 1u - Str_Len(p_line));
}
#endif

/*
*********************************************************************************************************
*                                          App_ErrCheck()
*
* Description : Central check of the error code returned by an OS call.
*
* Argument(s) : err     Error code of the OS call.
*
* Return(s)   : none
*
* Caller(s)   : all application functions after an OS call.
*
* Note(s)     : (1) Expected errors (e.g. OS_ERR_PEND_WOULD_BLOCK) are handled by the caller before.
*                   Everything else is a programming error: CPU_SW_EXCEPTION() stops here, the call
*                   stack in the debugger shows the faulty call.
*********************************************************************************************************
*/

static void App_ErrCheck(OS_ERR err)
{
    if (err != OS_ERR_NONE)                                     /* Unexpected error -> stop, see Note 1 */
    {
        CPU_SW_EXCEPTION(;);
    }
}

/* END OF FILE */