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
#include <os_cfg_app.h>                                         /* OS_CFG_TICK_RATE_HZ for ms -> ticks   */


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define  USE_BSP_TOGGLE                             1u

#define  APP_ENV_STARTUP_DLY_MS                     3u          /* BME688 start-up time 2 ms (datasheet  */
                                                                /* table 1) + 1 tick margin, see Note 2  */
                                                                /* of App_EnvInit()                  */
#define  APP_ENV_MEAS_DLY_MS                      100u          /* First wait after start: heater-on     */
                                                                /* time of heater step 0 (100 ms)        */
#define  APP_ENV_POLL_DLY_MS                       10u          /* Then check every 10 ms for new data   */
#define  APP_ENV_POLL_MAX                          20u          /* Give up after 20 checks (200 ms)      */

#define  APP_MS_PER_S                            1000u
#define  APP_MS_TO_TICKS(ms)                     (((ms) * OS_CFG_TICK_RATE_HZ) / APP_MS_PER_S)
#define  APP_PEND_FOREVER                  (OS_TICK)0u          /* Timeout 0 = wait forever              */
 
#define  APP_ENV_PERIOD_MS                       3000u          /* One measurement every 3 s             */
 
                                                                /* ---- Acceptable indoor range -------- */
#define  APP_ENV_TEMP_MIN_C100          ((CPU_INT32S)1900)      /* 19.00 degC  AStV par. 28 (1) Z 1      */
#define  APP_ENV_TEMP_MAX_C100          ((CPU_INT32S)2500)      /* 25.00 degC  AStV par. 28 (1) Z 1      */
#define  APP_ENV_HUM_MIN_M1000                  40000u          /* 40.000 %rH  AStV par. 28 (5) Z 1      */
#define  APP_ENV_HUM_MAX_M1000                  70000u          /* 70.000 %rH  AStV par. 28 (5) Z 1      */
#define  APP_ENV_PRESS_MIN_PA                   95000u          /*  950 hPa    plausibility only         */
#define  APP_ENV_PRESS_MAX_PA                  105000u          /* 1050 hPa    plausibility only         */
 
#define  APP_PERCENT                              100u
#define  APP_ENV_GAS_WARMUP_CNT                     5u          /* Gas: first 5 values only baseline     */
#define  APP_ENV_GAS_MIN_PCT                       50u          /* Bad air: R < 50 % of the baseline     */

                                                                /* ---- Event flags of App_Flags ------- */
                                                                /* Interface between the tasks, Note 1   */
#define  APP_FLAG_MEAS_ACTIVE              (OS_FLAGS)0x01u      /* Set/cleared by button: measuring on   */
#define  APP_FLAG_ENV_VALID                (OS_FLAGS)0x02u      /* Set by env task: values are valid     */
#define  APP_FLAG_ENV_OK                   (OS_FLAGS)0x04u      /* Set by env task: room climate is OK   */
#define  APP_FLAG_ENV_ALL                  (APP_FLAG_ENV_VALID | APP_FLAG_ENV_OK)
 
#define  APP_FLAGS_INIT                    (OS_FLAGS)0x00u      /* All flags cleared after reset         */

/* Note 1: Event flags are used instead of a semaphore because several tasks only
*          READ the state (measuring on/off, room climate OK). A pend without OS_OPT_PEND_FLAG_CONSUME does
*          not change the flags, so no task "steals" the event from another one.
*/

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

OS_SEM button_sem;

static  OS_FLAG_GRP  App_Flags;                                 /* Shared state of the application      */


static  OS_TCB   App_TaskStartTCB;
static  CPU_STK  App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB   App_TaskLED_TCB;
static  CPU_STK  App_TaskLEDStk[APP_CFG_TASK_LED_STK_SIZE];

static  OS_TCB   App_TaskButton_TCB;
static  CPU_STK  App_TaskButtonStk[APP_CFG_TASK_BUTTON_STK_SIZE];

static  OS_TCB   App_TaskEnvTCB;
static  CPU_STK  App_TaskEnvStk[APP_CFG_TASK_ENV_STK_SIZE];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_TaskStart  (void *p_arg);

static  void  App_TaskLED (void *p_arg);
static  void  App_TaskBUTTON (void *p_arg);

static  void  App_TaskCreate (void);
static  void  App_ObjCreate  (void);

static  void         App_TaskEnv     (void *p_arg);
static  CPU_BOOLEAN  App_EnvInit     (ENV_CALIB *p_calib);
static  CPU_BOOLEAN  App_EnvMeasure  (const ENV_CALIB *p_calib, ENV_DATA *p_data);
static  CPU_BOOLEAN  App_EnvEvaluate (const ENV_DATA *p_data, CPU_INT32U *p_gas_base, CPU_INT08U *p_gas_cnt);

static  void  App_ErrCheck (OS_ERR  err);

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

int  main (void)
{
  OS_ERR  os_err;


  BSP_PreInit();                                              /* Perform BSP pre-initialization.                      */

  CPU_Init();                                                 /* Initialize the uC/CPU services                       */

  OSInit(&os_err);                                            /* Init uC/OS-III.                                      */

  OSTaskCreate((OS_TCB      *)&App_TaskStartTCB,              /* Create the start task                                */
               (CPU_CHAR    *)"Start",
               (OS_TASK_PTR  )App_TaskStart, 
               (void        *)0,
               (OS_PRIO      )APP_CFG_TASK_START_PRIO,
               (CPU_STK     *)&App_TaskStartStk[0],
               (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE_LIMIT,
               (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE,
               (OS_MSG_QTY   )0u,
               (OS_TICK      )0u,
               (void        *)0,
               (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR      *)&os_err);

  OSStart(&os_err);                                            /* Start multitasking (i.e. give control to uC/OS-III).  */
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

static  void  App_TaskStart (void *p_arg)
{
  OS_ERR       err;
     
 (void)p_arg;

  BSP_PostInit();                                               /* Perform BSP post-initialization functions.       */
  
  BSP_CPU_TickInit();                                           /* Perfrom Tick Initialization                      */

#if (OS_CFG_STAT_TASK_EN > 0u)
  OSStatTaskCPUUsageInit(&err);
  App_ErrCheck(err);
#endif
 
#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif
 
  App_ObjCreate();                                              /* Kernel objects BEFORE the tasks that */
                                                                /* use them                             */
  App_TaskCreate();                                             /* Create application tasks.            */
 
                                                                /* TEST ONLY until TheAnt's button task */
                                                                /* sets this flag: start measuring      */
  OSFlagPost(&App_Flags, APP_FLAG_MEAS_ACTIVE, OS_OPT_POST_FLAG_SET, &err);
  App_ErrCheck(err);
 
  while (DEF_TRUE) {                                            /* Task body, always written as an infinite loop.   */
    OSTimeDlyHMSM(0, 0, 0, 100,
                  OS_OPT_TIME_HMSM_STRICT,
                  &err);
    App_ErrCheck(err);
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

static  void  App_TaskCreate (void)
{
  /* declare and define function local variables */
  OS_ERR  os_err;
  
  /* create LED task */
  OSTaskCreate((OS_TCB      *)&App_TaskLED_TCB,
               (CPU_CHAR    *)"TaskLED",
               (OS_TASK_PTR  )App_TaskLED, 
               (void        *)0,
               (OS_PRIO      )APP_CFG_TASK_LED_PRIO,
               (CPU_STK     *)&App_TaskLEDStk[0],
               (CPU_STK_SIZE )APP_CFG_TASK_LED_STK_SIZE_LIMIT,
               (CPU_STK_SIZE )APP_CFG_TASK_LED_STK_SIZE,
               (OS_MSG_QTY   )0u,
               (OS_TICK      )0u,
               (void        *)0,
               (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR      *)&os_err);
  App_ErrCheck(os_err);

  /* create Button task */
  OSTaskCreate((OS_TCB      *)&App_TaskButton_TCB,
               (CPU_CHAR    *)"TaskBUT",
               (OS_TASK_PTR  )App_TaskBUTTON, 
               (void        *)0,
               (OS_PRIO      )APP_CFG_TASK_BUTTON_PRIO,
               (CPU_STK     *)&App_TaskButtonStk[0],
               (CPU_STK_SIZE )APP_CFG_TASK_BUTTON_STK_SIZE_LIMIT,
               (CPU_STK_SIZE )APP_CFG_TASK_BUTTON_STK_SIZE,
               (OS_MSG_QTY   )0u,
               (OS_TICK      )0u,
               (void        *)0,
               (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR      *)&os_err);
  App_ErrCheck(os_err);
  
  /* create environment task */
  OSTaskCreate((OS_TCB      *)&App_TaskEnvTCB,
               (CPU_CHAR    *)"TaskENV",
               (OS_TASK_PTR  )App_TaskEnv,
               (void        *)0,
               (OS_PRIO      )APP_CFG_TASK_ENV_PRIO,
               (CPU_STK     *)&App_TaskEnvStk[0],
               (CPU_STK_SIZE )APP_CFG_TASK_ENV_STK_SIZE_LIMIT,
               (CPU_STK_SIZE )APP_CFG_TASK_ENV_STK_SIZE,
               (OS_MSG_QTY   )0u,
               (OS_TICK      )0u,
               (void        *)0,
               (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR      *)&os_err);
  App_ErrCheck(os_err);
 
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

static  void  App_ObjCreate (void)
{
  /* declare and define function local variables */
  OS_ERR  os_err;
 
  /* create button semaphore to synchronize button press events */
  OSSemCreate(&button_sem,"SEM_BUT",0,&os_err);
  App_ErrCheck(os_err);
 
  /* create event flag group shared by button, environment and LED tasks */
  OSFlagCreate(&App_Flags, "FLG_APP", APP_FLAGS_INIT, &os_err);
  App_ErrCheck(os_err);
}


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
* Note(s)     : (1) Expected errors (e.g. OS_ERR_TIMEOUT) must be handled by the caller BEFORE calling
*                   this function. Everything else is a programming error: CPU_SW_EXCEPTION() (uC-CPU,
*                   cpu_core.h) stops here, so a breakpoint on it shows the faulty call in the call stack.
*********************************************************************************************************
*/
 
static  void  App_ErrCheck (OS_ERR  err)
{
  if (err != OS_ERR_NONE) {                                     /* Unexpected error -> stop, see Note 1 */
    CPU_SW_EXCEPTION(;);
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
* Note(s)     : (1) All data (calibration, measured values, gas baseline) are local variables on the
*                   task stack: only this task needs them, so no global variable is necessary.
*               (2) The flags are only READ by other tasks, therefore no OS_OPT_PEND_FLAG_CONSUME.
*                   First a NON-blocking check: if measuring is off, the ENV flags are cleared (the LED must
*                   not show old values) and the task then waits BLOCKING until measuring is on again.
*                   So there is still exactly one endless loop and the CPU is released while waiting.
*               (3) Sensor errors (no answer, no new data) are an expected runtime case (e.g. bad contact):
*                   no exception, the ENV flags are cleared and the sensor is initialised again in the next
*                   cycle. Only OS errors are programming errors -> App_ErrCheck().
*               (4) The flags are posted so that other tasks never see "VALID + OK" for bad values:
*                   for bad values OK is cleared first and only then VALID is set.
*********************************************************************************************************
*/
 
static  void  App_TaskEnv (void *p_arg)
{
  OS_ERR       err;
  CPU_TS       ts;
  ENV_CALIB    calib;
  ENV_DATA     data;
  CPU_INT32U   gas_base;
  CPU_INT08U   gas_cnt;
  CPU_BOOLEAN  env_ready;
 
 
  (void)p_arg;
 
  gas_base  = 0u;                                               /* No gas baseline yet                  */
  gas_cnt   = 0u;
  env_ready = App_EnvInit(&calib);                              /* Check sensor, calibration, config    */
 
  while (DEF_TRUE) {
                                                                /* Measuring on? (Note 2)               */
    OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,
               (OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_NON_BLOCKING), &ts, &err);
    if (err == OS_ERR_PEND_WOULD_BLOCK) {                       /* No: clear old result ...             */
      OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_CLR, &err);
      App_ErrCheck(err);
      OSFlagPend(&App_Flags, APP_FLAG_MEAS_ACTIVE, APP_PEND_FOREVER,  /* ... and wait until it is on    */
                 (OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_BLOCKING), &ts, &err);
    }
    App_ErrCheck(err);
 
    if (env_ready == DEF_FAIL) {                                /* Sensor not ready: try again (Note 3) */
      env_ready = App_EnvInit(&calib);
    }
 
    if ((env_ready == DEF_OK) && (App_EnvMeasure(&calib, &data) == DEF_OK)) {
      if (App_EnvEvaluate(&data, &gas_base, &gas_cnt) == DEF_OK) {
        OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_SET, &err);    /* Valid + OK          */
      } else {
        OSFlagPost(&App_Flags, APP_FLAG_ENV_OK, OS_OPT_POST_FLAG_CLR, &err);     /* Not OK (Note 4) ... */
        App_ErrCheck(err);
        OSFlagPost(&App_Flags, APP_FLAG_ENV_VALID, OS_OPT_POST_FLAG_SET, &err);  /* ... but valid       */
      }
      App_ErrCheck(err);
    } else {                                                    /* Measurement failed (Note 3)          */
      env_ready = DEF_FAIL;
      OSFlagPost(&App_Flags, APP_FLAG_ENV_ALL, OS_OPT_POST_FLAG_CLR, &err);
      App_ErrCheck(err);
    }
 
    OSTimeDly(APP_MS_TO_TICKS(APP_ENV_PERIOD_MS), OS_OPT_TIME_DLY, &err);   /* Release CPU until next   */
    App_ErrCheck(err);                                                      /* measurement              */
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
* Note(s)     : (1) Each step is only executed if the previous one was successful.
*               (2) The sensor needs 2 ms after power-on before the first communication (datasheet,
*                   table 1, t_startup). An OS delay ends with a tick interrupt, so it can be up to one
*                   tick (1 ms) shorter than requested -> 3 ms guarantees at least 2 ms.
*********************************************************************************************************
*/
 
static  CPU_BOOLEAN  App_EnvInit (ENV_CALIB *p_calib)
{
  OS_ERR       os_err;
  ENV_ERR      env_err;
  CPU_BOOLEAN  result;
 
 
  OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_STARTUP_DLY_MS,             /* Wait for sensor start-up (Note 2)    */
                OS_OPT_TIME_HMSM_STRICT,
                &os_err);
  App_ErrCheck(os_err);
 
  env_err = env_check_id();                                     /* Read chip id + variant id            */
  if (env_err == ENV_ERR_NONE) {
    env_err = env_read_calib(p_calib);                          /* Read calibration values              */
  }
  if (env_err == ENV_ERR_NONE) {
    env_err = env_config(p_calib);                              /* Oversampling, filter, gas heater     */
  }
 
  if (env_err == ENV_ERR_NONE) {
    result = DEF_OK;
  } else {
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
* Note(s)     : (1) The datasheet gives no total measurement time. The task therefore waits at least the
*                   heater-on time with an OS delay and then checks new_data in short OS delays. The
*                   number of checks is limited, so the function cannot wait forever. The CPU is
*                   released during every wait ("Thou shalt release the CPU").
*               (2) The raw values are only needed here -> local variable.
*********************************************************************************************************
*/
 
static  CPU_BOOLEAN  App_EnvMeasure (const ENV_CALIB *p_calib, ENV_DATA *p_data)
{
  OS_ERR       os_err;
  ENV_ERR      env_err;
  ENV_RAW      raw;
  CPU_INT08U   poll;
  CPU_BOOLEAN  result;
 
 
  result       = DEF_FAIL;
  raw.new_data = DEF_NO;
 
  env_err = env_start_meas();                                   /* Start one measurement (forced mode)  */
  if (env_err == ENV_ERR_NONE) {
    OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_MEAS_DLY_MS,              /* Wait at least the heater-on time     */
                  OS_OPT_TIME_HMSM_STRICT,                      /* (Note 1)                             */
                  &os_err);
    App_ErrCheck(os_err);
 
    for (poll = 0u;                                             /* Check for new data, limited (Note 1) */
         (poll < APP_ENV_POLL_MAX) && (raw.new_data == DEF_NO) && (env_err == ENV_ERR_NONE);
         poll++) {
      env_err = env_read_raw(&raw);
      if ((env_err == ENV_ERR_NONE) && (raw.new_data == DEF_NO)) {  /* Not finished: release the CPU   */
        OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_POLL_DLY_MS,
                      OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
        App_ErrCheck(os_err);
      }
    }
 
    if ((env_err == ENV_ERR_NONE) && (raw.new_data == DEF_YES)) {
      result = env_calc_data(p_calib, &raw, p_data);            /* Convert into degC, Pa, %rH, Ohm      */
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
*               p_gas_cnt    Number of valid gas values so far (input/output, kept by the task).
*
* Return(s)   : DEF_OK    all values acceptable.
*               DEF_FAIL  at least one value outside the range.
*
* Caller(s)   : App_TaskEnv()
*
* Note(s)     : (1) Temperature 19-25 degC and humidity 40-70 %rH: Austrian workplace regulation
*                   (AStV par. 28). Pressure 950-1050 hPa: only a plausibility check, the weather is no
*                   indoor air quality problem.
*               (2) Gas: the datasheet (ch. 3.3) gives no absolute limit - more VOC lowers the resistance.
*                   The baseline is the highest resistance measured so far (= cleanest air). It rises with
*                   the burn-in of a new sensor. The first APP_ENV_GAS_WARMUP_CNT values only build the
*                   baseline. After that the air is bad if R < APP_ENV_GAS_MIN_PCT % of the baseline.
*               (3) (base / 100) * 50 instead of (base * 50) / 100: no overflow of CPU_INT32U even for
*                   very high resistances; the lost precision (< 100 Ohm) does not matter.
*               (4) Gas value not valid (heater not stable): this cycle the gas is not evaluated.
*********************************************************************************************************
*/
 
static  CPU_BOOLEAN  App_EnvEvaluate (const ENV_DATA *p_data, CPU_INT32U *p_gas_base, CPU_INT08U *p_gas_cnt)
{
  CPU_BOOLEAN  result;
 
 
  result = DEF_OK;
 
  if ((p_data->temp_c100 < APP_ENV_TEMP_MIN_C100) ||            /* Temperature (Note 1)                 */
      (p_data->temp_c100 > APP_ENV_TEMP_MAX_C100)) {
    result = DEF_FAIL;
  }
  if ((p_data->hum_m1000 < APP_ENV_HUM_MIN_M1000) ||            /* Humidity (Note 1)                    */
      (p_data->hum_m1000 > APP_ENV_HUM_MAX_M1000)) {
    result = DEF_FAIL;
  }
  if ((p_data->press_pa < APP_ENV_PRESS_MIN_PA) ||              /* Pressure (Note 1)                    */
      (p_data->press_pa > APP_ENV_PRESS_MAX_PA)) {
    result = DEF_FAIL;
  }
 
  if (p_data->gas_ok == DEF_YES) {                              /* Gas (Note 2, Note 4)                 */
    if (p_data->gas_ohm > *p_gas_base) {                        /* Cleaner air than before: new base    */
      *p_gas_base = p_data->gas_ohm;
    }
    if (*p_gas_cnt < APP_ENV_GAS_WARMUP_CNT) {                  /* Warm-up: only build the baseline     */
      (*p_gas_cnt)++;
    } else if (p_data->gas_ohm < ((*p_gas_base / APP_PERCENT) * APP_ENV_GAS_MIN_PCT)) {  /* Note 3     */
      result = DEF_FAIL;
    } else {
      ;                                                         /* Gas OK                               */
    }
  }
 
  return (result);
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


static  void  App_TaskLED (void *p_arg)
{
  /* declare and define task local variables */
  OS_ERR       os_err;
  CPU_TS         ts;
  
  /* prevent compiler warnings */
  (void)p_arg;
  
  /* start of the endless loop */
  while (DEF_TRUE) {
    /* pend on semaphore - blocking */
    OSSemPend(&button_sem,0,OS_OPT_PEND_BLOCKING,&ts,&os_err);
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


static  void  App_TaskBUTTON (void *p_arg)
{
  /* declare and define task local variables */
  OS_ERR       os_err;
  CPU_INT08S    button_status = 0;
  static CPU_BOOLEAN button_pressed = 0;
  
  /* prevent compiler warnings */
  (void)p_arg;
  
  /* start of the endless loop */ 
  while (DEF_TRUE) {
    /* check is button is pressed */
    button_status = BSP_PB_StatusGet(P2_2);
    /* check the button status (pressed ... 0 or not pressed ... 1) */
    if(button_status){
      /* if button not pressed reset button_pressed variable (Button State)*/
      button_pressed = 0;
    }
    else if(!button_status){
      /* if button is pressed check button_pressed variable */
      if(!button_pressed){
        /* if button_pressed (Button State) was not pressed before - post 
        semaphore */
        OSSemPost(&button_sem,OS_OPT_POST_ALL,&os_err);
      }
      else{
        /* if button_pressed (Button State) was pressed before - post semaphore
        and set to 1 that only one pend is accepted*/
        OSSemSet(&button_sem,1,&os_err);
      }
      /* in both cases set button_pressed (Button State) to 1 */
      button_pressed = 1;
    }
    else{
      /* error */
    }
    /* initiate scheduler */
    OSTimeDlyHMSM(0, 0, 0, 100, 
                  OS_OPT_TIME_HMSM_STRICT, 
                  &os_err);
  }
}

/* END OF FILE */