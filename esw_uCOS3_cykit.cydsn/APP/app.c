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

#define SPO2_ADDRESS                                0x57u //MAX30102 address from datasheet
#define MAX_ID_REG                                  0xFFu //MAX30102 Register for ID
#define MAX_ID                                      0x15u //MAX30102 ID from datasheet


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

static  OS_TCB   App_TaskSPO2_TCB;
static  CPU_STK  App_TaskSPO2Stk[APP_CFG_TASK_SPO2_STK_SIZE];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_TaskStart  (void *p_arg);

static  void  App_TaskLED (void *p_arg);
static  void  App_TaskBUTTON (void *p_arg);
static  void  App_TaskSPO2 (void *p_arg);

static  void  App_TaskCreate (void);
static  void  App_ObjCreate  (void);



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
  
  I2C_spo2_Start();
  Check_Sensor(SPO2_ADDRESS,MAX_ID_REG,MAX_ID);
  
    

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
  /* create SPO2 task */
  OSTaskCreate((OS_TCB      *)&App_TaskSPO2_TCB,
               (CPU_CHAR    *)"TaskSPO2",
               (OS_TASK_PTR  )App_TaskSPO2, 
               (void        *)0,
               (OS_PRIO      )APP_CFG_TASK_SPO2_PRIO,
               (CPU_STK     *)&App_TaskSPO2Stk[0],
               (CPU_STK_SIZE )APP_CFG_TASK_SPO2_STK_SIZE_LIMIT,
               (CPU_STK_SIZE )APP_CFG_TASK_SPO2_STK_SIZE,
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
static void App_TaskSPO2(void *p_arg){
    /* prevent compiler warnings */
    (void)p_arg;
    
    
    
    
}
/* END OF FILE */