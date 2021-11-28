/* ************************************************************
* Xenomai - creates a periodic task
*	
* Paulo Pedreiras
* 	Out/2020: Upgraded from Xenomai V2.5 to V3.1    
* 
************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include <sys/mman.h> // For mlockall

// Xenomai API (former Native API)
#include <alchemy/task.h>
#include <alchemy/timer.h>

#define MS_2_NS(ms)(ms*1000*1000) /* Convert ms to ns */
#define NS_IN_SEC 1000000000L

/* *****************************************************
 * Define task structure for setting input arguments
 * *****************************************************/
struct taskArgsStruct {
	RTIME taskPeriod_ns;
	int mode;
};

/* *******************
 * Task attributes 
 * *******************/ 
#define TASK_MODE 0  	// No flags
#define TASK_STKSZ 0 	// Default stack size

#define TASK_A_PRIO 25 	// RT priority [0..99]
#define TASK_PERIOD_NS MS_2_NS(1000)

RT_TASK task_a_desc; // Task decriptor
RT_TASK task_b_desc;
RT_TASK task_c_desc;

/* **********************
 * Semaphore attributes
 * **********************/
RT_SEM sem;

int N = 0;			//Shared memory


/* *********************
* Function prototypes
* **********************/
void catch_signal(int sig); 	/* Catches CTRL + C to allow a controlled termination of the application */
void wait_for_ctrl_c(void);
void Heavy_Work(void);      	/* Load task */
void task_code(void *args); 	/* Task body */
int changeAffinity(RT_TASK task1,RT_TASK task2); //Change affinity to CPU0 


/* *********************
* Change Affinity function
* **********************/
int changeAffinity(RT_TASK task1, RT_TASK task2){
	cpu_set_t cpuset;                                       //cpu_set bit mask.
	CPU_ZERO(&cpuset);                                      //Initialize it all to 0
	CPU_SET(0,&cpuset);                                     //Set the bit that represents core 0
	if(rt_task_set_affinity(&task1,&cpuset) || rt_task_set_affinity(&task2,&cpuset)) {     //Set thread's CPU affinity mask to 0 
		printf("\n Lock of process to CPU0 failed!!!");
	return(1);
	}
}


/* ******************
* Main function
* *******************/ 
int main(int argc, char *argv[]) {
	int err,err2,err3; 
	struct taskArgsStruct taskAArgs;
	struct taskArgsStruct taskBArgs;
	struct taskArgsStruct taskCArgs;

	
	/* Lock memory to prevent paging */
	mlockall(MCL_CURRENT|MCL_FUTURE); 

	/* Create RT task */
	/* Args: descriptor, name, stack size, priority [0..99] and mode (flags for CPU, FPU, joinable ...) */
	err=rt_task_create(&task_a_desc, "Task a", TASK_STKSZ, TASK_A_PRIO, TASK_MODE);
    err2=rt_task_create(&task_b_desc, "Task b", TASK_STKSZ, 10, TASK_MODE);
    err3=rt_task_create(&task_c_desc, "Task c", TASK_STKSZ, 75, TASK_MODE);
	if(err || err2 || err3) {
        if(err){
            printf("Error creating task a (error code = %d)\n",err);
		    return err;
        }
        else if(err2){
            printf("Error creating task a (error code = %d)\n",err2);
		    return err2;
        }
        else if(err3){
            printf("Error creating task a (error code = %d)\n",err3);
		    return err3;
        }
	} else 
		printf("Task a created successfully\n");
	
			
	/* Start RT task */
	/* Args: task decriptor, address of function/implementation and argument*/
	taskAArgs.taskPeriod_ns = TASK_PERIOD_NS;
	taskAArgs.mode = 1;
	taskBArgs.taskPeriod_ns = TASK_PERIOD_NS; 
	printf("%d", taskBArgs.mode);
	taskCArgs.taskPeriod_ns = TASK_PERIOD_NS;

	changeAffinity(task_b_desc,task_c_desc);

	rt_sem_create()

    rt_task_start(&task_a_desc, &task_code, (void *)&taskAArgs);
	rt_task_start(&task_b_desc, &task_code, (void *)&taskBArgs);
	rt_task_start(&task_c_desc, &task_code, (void *)&taskCArgs);
    
	/* wait for termination signal */	
	wait_for_ctrl_c();

	return 0;
		
}


/* ***********************************
* Task body implementation
* *************************************/
void task_code(void *args) {
	RT_TASK *curtask;
	RT_TASK_INFO curtaskinfo;
	struct taskArgsStruct *taskArgs;

	RTIME ta=0;
	unsigned long overruns;
	int err;
	
	/* Get task information */
	curtask=rt_task_self();
	rt_task_inquire(curtask,&curtaskinfo);
	taskArgs=(struct taskArgsStruct *)args;
	printf("Task %s init, period:%llu\n", curtaskinfo.name, taskArgs->taskPeriod_ns);
	int nitter = 0 ;
	int update = 0;
	RTIME p;
	RTIME last_ta;
	RTIME max_iat = 0;
	RTIME min_iat = LLONG_MAX ;

	/* Set task as periodic */
	err=rt_task_set_periodic(NULL, TM_NOW, taskArgs->taskPeriod_ns);
	err=rt_
	for(;;) {
		err=rt_task_wait_period(&overruns);
		ta=rt_timer_read();
		if(err) {
			printf("task %s overrun!!!\n", curtaskinfo.name);
			break;
		}
		printf("Task %s activation at time %llu\n", curtaskinfo.name,ta);
		nitter ++;
		if(nitter == 1){
			last_ta = ta;
			continue;
		}
		
		p = ta - last_ta ;
		if(p>max_iat){
			max_iat = p;
			update = 1;
		}
		if(p<min_iat){
			min_iat = p;
			update = 1;
		}
		last_ta = ta;

		if(update) {
		  printf("Task %s inter-arrival time: min: %llu / max: %llu\n\r",curtaskinfo.name, min_iat, max_iat);
		  update = 0;
		}

		/* Task "load" */
		Heavy_Work();
	}
	return;
}


/* **************************************************************************
 *  Catch control+c to allow a controlled termination
 * **************************************************************************/
 void catch_signal(int sig)
 {
	 return;
 }

void wait_for_ctrl_c(void) {
	signal(SIGTERM, catch_signal); //catch_signal is called if SIGTERM received
	signal(SIGINT, catch_signal);  //catch_signal is called if SIGINT received

	// Wait for CTRL+C or sigterm
	pause();
	
	// Will terminate
	printf("Terminating ...\n");
}


/* **************************************************************************
 *  Task load implementation. In the case integrates numerically a function
 * **************************************************************************/
#define f(x) 1/(1+pow(x,2)) /* Define function to integrate*/
void Heavy_Work(void)
{
	float lower, upper, integration=0.0, stepSize, k;
	int i, subInterval;
	
	RTIME ts, // Function start time
		  tf; // Function finish time
			
	static int first = 0; // Flag to signal first execution		
	
	/* Get start time */
	ts=rt_timer_read();
	
	/* Integration parameters */
	/*These values can be tunned to cause a desired load*/
	lower=0;
	upper=100;
	subInterval=1000000;

	 /* Calculation */
	 /* Finding step size */
	 stepSize = (upper - lower)/subInterval;

	 /* Finding Integration Value */
	 integration = f(lower) + f(upper);
	 for(i=1; i<= subInterval-1; i++)
	 {
		k = lower + i*stepSize;
		integration = integration + 2 * f(k);
 	}
	integration = integration * stepSize/2;
 	
 	/* Get finish time and show results */
 	if (!first) {
		tf=rt_timer_read();
		tf-=ts;  // Compute time difference form start to finish
        
		printf("Integration value is: %.3f. It took %9llu ns to compute.\n", integration, tf);
		
		first = 1;
	}

}