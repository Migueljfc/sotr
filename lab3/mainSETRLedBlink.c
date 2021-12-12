/*
 * Paulo Pedreiras, Sept/2021
 *
 * FREERTOS demo for ChipKit MAX32 board
 * - Creates two periodic tasks
 * - One toggles Led LD4, other is a long (interfering)task that 
 *      activates LD5 when executing 
 * - When the interfering task has higher priority interference becomes visible
 *      - LD4 does not blink at the right rate
 *
 * Environment:
 * - MPLAB X IDE v5.45
 * - XC32 V2.50
 * - FreeRTOS V202107.00
 *
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#include <xc.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"


/* App includes */
#include "../UART/uart.h"

/* Set the tasks' period (in system ticks) */
#define LED_FLASH_PERIOD_MS 	( 250 / portTICK_RATE_MS ) // 
#define INTERF_PERIOD_MS 	( 3000 / portTICK_RATE_MS )

/* Control the load task execution time (# of iterations)*/
/* Each unit corresponds to approx 50 ms*/
#define INTERF_WORKLOAD          ( 20)

/* Priorities of the demo application tasks (high numb. -> high prio.) */
#define LED_FLASH_PRIORITY	( tskIDLE_PRIORITY + 1)
#define INTERF_PRIORITY	    ( tskIDLE_PRIORITY + 2)

/*
 * Prototypes and tasks
 */

void pvLedFlash(void *pvParam)
{
    int iTaskTicks = 0;
    uint8_t mesg[80];
    
    for(;;) {
        PORTAbits.RA3 = !PORTAbits.RA3;
        sprintf(mesg,"Task LedFlash (job %d)\n\r",iTaskTicks++);
        PrintStr(mesg);
        vTaskDelay(LED_FLASH_PERIOD_MS);        
    }
}

void vLedFlash(void *pvParam)
{
    int iTaskTicks = 0;
    uint8_t mesg[80];
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = portTICK_PERIOD_MS*LED_FLASH_PERIOD_MS;

    //const TickType_t xFrequency = 20;
    xLastWakeTime = xTaskGetTickCount();
    for(;;) {
              
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
        PORTAbits.RA3 = !PORTAbits.RA3;
        sprintf(mesg,"Task LedFlash (job %d)\n\r",iTaskTicks++);
        PrintStr(mesg); 
       
    }
}


void vInterfTask(void *pvParam)
{
    
    volatile uint32_t counter1, counter2;
    float x=100.1;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = portTICK_PERIOD_MS*INTERF_PERIOD_MS;

    //const TickType_t xFrequency = 1/Le;
    xLastWakeTime = xTaskGetTickCount();  
    for(;;) {       
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
        PORTCbits.RC1 = 1;        
        PrintStr("Interfering task release ...");
        
        /* Workload. In this case just spend CPU time ...*/        
        for(counter1=0; counter1 < INTERF_WORKLOAD; counter1++ )
            for(counter2=0; counter2 < 0x10200; counter2++ )
            x=x/3;                

        PrintStr("and termination!\n\r");
        PORTCbits.RC1 = 0;        
         
       
    }
}

void pvInterfTask(void *pvParam)
{
    
    volatile uint32_t counter1, counter2;
    float x=100.1;
            
    for(;;) {
        PORTCbits.RC1 = 1;        
        PrintStr("Interfering task release ...");
        
        /* Workload. In this case just spend CPU time ...*/        
        for(counter1=0; counter1 < INTERF_WORKLOAD; counter1++ )
            for(counter2=0; counter2 < 0x10200; counter2++ )
            x=x/3;                

        PrintStr("and termination!\n\r");
        PORTCbits.RC1 = 0;        
        
        vTaskDelay(INTERF_PERIOD_MS);         
    }
}


/*
 * Create the demo tasks then start the scheduler.
 */
int mainSetrLedBlink( void )
{
    
    // Set RA3 (LD4) and RC1 (LD5) as outputs
    TRISAbits.TRISA3 = 0;
    TRISCbits.TRISC1 = 0;
    PORTAbits.RA3 = 0;
    PORTCbits.RC1 = 0;

	// Init UART and redirect stdin/stdot/stderr to UART
    if(UartInit(configPERIPHERAL_CLOCK_HZ, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }

     __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
    
    /* Welcome message*/
    printf("\n\n *********************************************\n\r");
    printf("Starting SETR FreeRTOS Demo - Simple Led Blink\n\r");
    printf("*********************************************\n\r");
    
      
    /* Create the tasks defined within this file. */
	xTaskCreate( vLedFlash, ( const signed char * const ) "Flash", configMINIMAL_STACK_SIZE, NULL, LED_FLASH_PRIORITY, NULL );
    xTaskCreate( vInterfTask, ( const signed char * const ) "Interf", configMINIMAL_STACK_SIZE, NULL, INTERF_PRIORITY, NULL );

        /* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	the scheduler. */
	return 0;
}
