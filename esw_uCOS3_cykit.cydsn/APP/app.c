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

#define  APP_USER_IF_SIGN_ON                        0u
#define  APP_USER_IF_VER_TICK_RATE                  1u
#define  APP_USER_IF_CPU                            2u
#define  APP_USER_IF_CTXSW                          3u
#define  APP_USER_IF_STATE_MAX                      4u

#define  USE_BSP_TOGGLE                             1u

#define  APP_ENV_STARTUP_DLY_MS                     3u          /* BME688 start-up time 2 ms (datasheet  */
                                                                /* table 1) + 1 tick margin, see Note 2  */
                                                                /* of App_EnvSelfTest()                  */
#define  APP_ENV_MEAS_DLY_MS                      100u          /* First wait after start: heater-on     */
                                                                /* time of heater step 0 (100 ms)        */
#define  APP_ENV_POLL_DLY_MS                       10u          /* Then check every 10 ms for new data   */
#define  APP_ENV_POLL_MAX                          20u          /* Give up after 20 checks (200 ms)      */

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

OS_SEM button_sem;

static  OS_TCB   App_TaskStartTCB;
static  CPU_STK  App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  OS_TCB   App_TaskLED_TCB;
static  CPU_STK  App_TaskLEDStk[APP_CFG_TASK_LED_STK_SIZE];

static  OS_TCB   App_TaskButton_TCB;
static  CPU_STK  App_TaskButtonStk[APP_CFG_TASK_BUTTON_STK_SIZE];


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

static  void  App_EnvSelfTest (void);


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
#endif    

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif      

  App_EnvSelfTest();                                            /* Check BME688 on SPI                  */

  App_TaskCreate();                                             /* Create application tasks.                        */

  App_ObjCreate();                                              /* Create kernel objects                            */

  while (DEF_TRUE) {                                            /* Task body, always written as an infinite loop.   */
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
}


/*
*********************************************************************************************************
*                                          App_EnvSelfTest()
*
* Description : Checks once at start-up whether the BME688 (Environment 3 Click, slot 1) answers on the
*               SPI bus, reads its calibration values, performs one measurement and converts the raw
*               values into degC, Pa, %rH and Ohm.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : App_TaskStart().
*
* Note(s)     : (1) Test function for the first hardware tests of the SPI driver. It will be replaced
*                   by the environment task.
*               (2) The sensor needs 2 ms after power-on before the first communication (datasheet,
*                   table 1, t_startup). An OS delay ends with a tick interrupt, so it can be up to one
*                   tick (1 ms) shorter than requested -> 3 ms guarantees at least 2 ms.
*               (3) Check the result with a breakpoint on the marked lines (env_err == ENV_ERR_NONE)
*                   and with the logic analyzer (MISO answers 0x61 to the control byte 0xD0).
*               (4) The calibration values are a local variable: no global variable is needed, the
*                   environment task will later keep them the same way in its own stack.
*               (5) The datasheet gives no total measurement time. The task therefore waits at least the
*                   heater-on time with an OS delay and then checks new_data in short OS delays. The
*                   number of checks is limited, so the function cannot wait forever. The CPU is
*                   released during every wait ("Thou shalt release the CPU").
*********************************************************************************************************
*/
 
static  void  App_EnvSelfTest (void)
{
  OS_ERR      os_err;
  ENV_ERR     env_err;
  ENV_CALIB   calib;
  ENV_RAW     raw;
  ENV_DATA    data;
  CPU_INT08U  poll;
 
 
  OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_STARTUP_DLY_MS,             /* Wait for sensor start-up (Note 2)    */
                OS_OPT_TIME_HMSM_STRICT,
                &os_err);
  if (os_err != OS_ERR_NONE) {
    CPU_SW_EXCEPTION(;);                                        /* Unexpected OS error: stop here       */
  }
 
  env_err = env_check_id();                                     /* Read chip id + variant id            */
  if (env_err != ENV_ERR_NONE) {                                /* Breakpoint here (Note 3)             */
    CPU_SW_EXCEPTION(;);                                        /* Sensor not found: stop here          */
  }
 
  env_err = env_read_calib(&calib);                             /* Read calibration values (Note 4)     */
  if (env_err != ENV_ERR_NONE) {
    CPU_SW_EXCEPTION(;);                                        /* Read failed: stop here               */
  }
 
  env_err = env_config(&calib);                                 /* Oversampling, filter, gas heater     */
  if (env_err != ENV_ERR_NONE) {
    CPU_SW_EXCEPTION(;);
  }
 
  env_err = env_start_meas();                                   /* Start one measurement (forced mode)  */
  if (env_err != ENV_ERR_NONE) {
    CPU_SW_EXCEPTION(;);
  }
 
  OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_MEAS_DLY_MS,                /* Wait at least the heater-on time     */
                OS_OPT_TIME_HMSM_STRICT,                        /* (Note 5)                             */
                &os_err);
  if (os_err != OS_ERR_NONE) {
    CPU_SW_EXCEPTION(;);
  }
 
  raw.new_data = DEF_NO;                                        /* Check for new data, limited (Note 5) */
  for (poll = 0u; (poll < APP_ENV_POLL_MAX) && (raw.new_data == DEF_NO); poll++) {
    env_err = env_read_raw(&raw);
    if (env_err != ENV_ERR_NONE) {
      CPU_SW_EXCEPTION(;);
    }
    if (raw.new_data == DEF_NO) {                               /* Not finished yet: release the CPU    */
      OSTimeDlyHMSM(0u, 0u, 0u, APP_ENV_POLL_DLY_MS,
                    OS_OPT_TIME_HMSM_STRICT,
                    &os_err);
      if (os_err != OS_ERR_NONE) {
        CPU_SW_EXCEPTION(;);
      }
    }
  }
 
  if (raw.new_data == DEF_NO) {                                 /* Breakpoint here: check "raw", "poll" */
    CPU_SW_EXCEPTION(;);                                        /* No data within the time limit        */
  }
  if (env_calc_data(&calib, &raw, &data) != DEF_OK) {           /* Convert into degC, Pa, %rH, Ohm      */
    CPU_SW_EXCEPTION(;);                                        /* Breakpoint on this "if" + F10:       */
  }                                                             /* then check "data"                    */
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

/*
*********************************************************************************************************
*                                          App_TaskCOM()
*
* Description : COM Task checks for available bytes within the UART receive buffer. If correct string is
*               available (e.g. PC -> uC: #abc$ or #Hellor World$), process the message and output a
*               pre-defined string via UART and append the user-defined string via the UART interface.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskCOM()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  App_TaskCOM (void *p_arg)
{
  /* declare and define task local variables */
  OS_ERR       os_err;
  CPU_INT08U   rec_byte = 0x00;
  CPU_INT08U   rx_msg[UART_1_RX_BUFFER_SIZE] = {0};
  CPU_INT08U   idx = 0x00;
  CPU_INT08U   rec_byte_cnt = 0x00;
  CPU_BOOLEAN  str_available = DEF_FALSE;
  
  /* prevent compiler warnings */
  (void)p_arg;
  (void)Start_of_Packet;
  (void)End_of_Packet;
  
  /* start of the endless loop */
  while (DEF_TRUE) {
    /* check if a byte is available */
    rec_byte = uart_get_byte();
    /* check if the received byte is '#'*/
    if(rec_byte == Start_of_Packet){
      /* if received byte was correct */
      while(DEF_TRUE){
        /* receive byte by byte */
        rec_byte = uart_get_byte();
        /* check is byte is something meaningful */
        if(rec_byte){
          /* save byte into software receive buffer and increment idx */
          rx_msg[idx++] = rec_byte;
        }
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 0, 20, 
                      OS_OPT_TIME_HMSM_STRICT, 
                      &os_err);
        /* check if received byte is '$' */
        if(rx_msg[idx-1]==End_of_Packet){
          /* if end of packet is reached -> break */
          break;
        }
      }
      /* message received, calculate received bytes, -2 because of '#' & '$' */
      rec_byte_cnt = idx-2;
      /* signal that a string is available */
      str_available = DEF_TRUE;
    }
    /* if received byte wasn't start of packet */
    else{
      /* reset software receive buffer */
      memset(&rx_msg[0],0,sizeof(rx_msg));
      /* reset string available signal */
      str_available = DEF_FALSE;
      /* reset received byte variable */
      rec_byte = 0x00;
      /* reset idx */
      idx = 0x00;
    }
    /* check if message is available */
    if(str_available){
      /* send pre-defined bytes via UART interface */
      uart_send_byte('C');
      uart_send_byte('Y');
      uart_send_byte('8');
      uart_send_byte('K');
      uart_send_byte('I');
      uart_send_byte('T');
      uart_send_byte(':');
      /* send received message without '#' and '$' */
      for(idx=0;idx<=rec_byte_cnt;idx++){
        uart_send_byte(rx_msg[idx]);
      }
      /* reset software receive buffer */
      memset(&rx_msg[0],0,sizeof(rx_msg));
      /* reset string available signal */
      str_available = DEF_FALSE;
      /* reset received byte variable */
      rec_byte = 0x00;
      /* reset idx */
      idx = 0x00;
    }
    /* initiate scheduler */
    OSTimeDlyHMSM(0, 0, 0, 100, 
                  OS_OPT_TIME_HMSM_STRICT, 
                  &os_err);
  }
}

/* END OF FILE */