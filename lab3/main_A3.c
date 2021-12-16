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
#include <semphr.h>

/* Set the tasks' period (in system ticks) */
#define PERIODIC_TASK_MS 	( 100 / portTICK_RATE_MS )


/* Control the load task execution time (# of iterations)*/
/* Each unit corresponds to approx 50 ms*/
#define INTERF_WORKLOAD          ( 20)

/* Priorities of the demo application tasks (high numb. -> high prio.) */
#define ACQ_PRIORITY        ( tskIDLE_PRIORITY + 3 )
#define PROC_PRIORITY	    ( tskIDLE_PRIORITY + 2 )
#define OUT_PRIORITY	    ( tskIDLE_PRIORITY + 1 )

/*
 * Global Variables
 */
SemaphoreHandle_t Sem1;
SemaphoreHandle_t Sem2;

int x1, x2 = 0;

/*
 * Prototypes and tasks
 */

void vAcqTask(void *pvParam)
{
    uint8_t mesg[80];
    
    
    int iTaskTicks, res = 0;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(PERIODIC_TASK_MS);
    
    xLastWakeTime = xTaskGetTickCount();
    for(;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Get one sample
        IFS1bits.AD1IF = 0; // Reset interrupt flag
        AD1CON1bits.ASAM = 1; // Start conversion
        while (IFS1bits.AD1IF == 0); // Wait fo EOC

        // Convert to 0..3.3V 
        res = (ADC1BUF0 * 3.3) / 1023;
        x1 = (res * 100) / 3.3; 
        xSemaphoreGive(Sem1);
    }
}

void vProcTask(void *pvParam)
{
    int val[5] = {0};
    int i = 0;
    
    
    uint8_t mesg[80];
    for(;;) {
        if (xSemaphoreTake(Sem1, ( TickType_t ) 10 ) == pdTRUE) {
            val[i] = x1;
            
            if (i == 4) {
                x2 = (val[0] + val[1] + val[2] + val[3] + val[4]) / 5;
                i = 0;
                xSemaphoreGive(Sem2);
            } else {
                i++;
            }
        }
    }
}

void vOutTask(void *pvParam)
{
    uint8_t mesg[80];
    
    for(;;) {
        if (xSemaphoreTake(Sem2, ( TickType_t ) 10 ) == pdTRUE) {
        sprintf(mesg,"Task Out (job)\n\r Mean Temp: %d\n\r", x2);
        
        PrintStr(mesg);     
        }
    }
}

/*
 * Create the demo tasks then start the scheduler.
 */
int main_A3( void )
{
    
    // Disable JTAG interface as it uses a few ADC ports
    DDPCONbits.JTAGEN = 0;
    
    // Initialize ADC module
    // Polling mode, AN0 as input
    // Generic part
    AD1CON1bits.SSRC = 7; // Internal counter ends sampling and starts conversion
    AD1CON1bits.CLRASAM = 1; //Stop conversion when 1st A/D converter interrupt is generated and clears ASAM bit automatically
    AD1CON1bits.FORM = 0; // Integer 16 bit output format
    AD1CON2bits.VCFG = 0; // VR+=AVdd; VR-=AVss
    AD1CON2bits.SMPI = 0; // Number (+1) of consecutive conversions, stored in ADC1BUF0...ADCBUF{SMPI}
    AD1CON3bits.ADRC = 1; // ADC uses internal RC clock
    AD1CON3bits.SAMC = 16; // Sample time is 16TAD ( TAD = 100ns)
    // Set AN0 as input
    AD1CHSbits.CH0SA = 0; // Select AN0 as input for A/D converter
    TRISBbits.TRISB0 = 1; // Set AN0 to input mode
    AD1PCFGbits.PCFG0 = 0; // Set AN0 to analog mode
    // Enable module
    AD1CON1bits.ON = 1; // Enable A/D module (This must be the ***last instruction of configuration phase***)

    
    Sem1 = xSemaphoreCreateBinary();
    Sem2 = xSemaphoreCreateBinary();

	// Init UART and redirect stdin/stdot/stderr to UART
    if(UartInit(configPERIPHERAL_CLOCK_HZ, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }

     __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
    

    /* Create the tasks defined within this file. */
	xTaskCreate( vAcqTask, ( const signed char * const ) "Acquisition", configMINIMAL_STACK_SIZE, NULL, ACQ_PRIORITY, NULL );
    xTaskCreate( vProcTask, ( const signed char * const ) "Processing", configMINIMAL_STACK_SIZE, NULL, PROC_PRIORITY, NULL );
    xTaskCreate( vOutTask, ( const signed char * const ) "Out", configMINIMAL_STACK_SIZE, NULL, OUT_PRIORITY, NULL );
    
    /* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	the scheduler. */
	return 0;
}
