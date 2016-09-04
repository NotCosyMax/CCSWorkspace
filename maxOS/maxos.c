#include <msp430g2553.h>
#include <stdio.h>
#include <stdlib.h>
#include "maxos.h"

//TYPE DEFINES
typedef struct taskList* 	taskList_t;
//Single linked task list
struct taskList {

	taskStruct_t 	pTask;

	taskList_t		pNext;

};




//OS parameter struct (only one instance should exist of this in run time)
struct osParameters {

	// CPU clock speed
	unsigned int			fcpu;

	// OS time resolution (res in high, counter in low)
	unsigned int			timeResolution;

	// Current time since OS start
	unsigned long long 		time;

	// Sleep until time
	unsigned long long 		sleepUntil;

	// Task that triggered sleep
	taskStruct_t			sleepUntilTask;

	// Number of created task
	unsigned char 			numberOfTasks;

	// Scheduled task
	taskStruct_t			runningTask;

	// "Task Ready" bitvector
	unsigned int			tasksReady;

	// "Premtive task" bitvector
	unsigned int			preemptiveTask;

	// Pre-empted task list
	taskList_t				pPreemptingTasks;

	//Tasks lists
	taskList_t				pTaskList;

#ifdef TDCMODULE
	unsigned int			tdcValue;
	unsigned char			tdcRunning:2;
	unsigned char			tdcStartTriggerEdge:1;
	unsigned char			tdcStopTriggerEdge:1;


#endif

#ifdef PWMMODULE
	unsigned int			pwmActive;
	unsigned int			pwmInactive;
#endif

};


static struct osParameters  osParams = {0};

//OTHER STUFF


//Used for "pre-emption"
asm volatile("	.bss	c_asm_bridge,4,4");
asm volatile("	.global c_asm_bridge");
asm volatile("	.bss	preemptive_func,4,4");
asm volatile("	.global preemptive_func");
asm volatile("	.bss	sr_save,2,2");
asm volatile("	.global sr_save");

#ifdef TDCMODULE
asm volatile("	.bss	tdcStartTriggerPin,2,2");
asm volatile("	.global tdcStartTriggerPin");
asm volatile("	.bss	tdcStopTriggerPin,2,2");
asm volatile("	.global tdcStopTriggerPin");
asm volatile("	.bss	tdcStartPortOffset,4,4");
asm volatile("	.global tdcStartPortOffset");
asm volatile("	.bss	tdcStopPortOffset,4,4");
asm volatile("	.global tdcStopPortOffset");

extern unsigned char 	tdcStartTriggerPin;
extern unsigned char 	tdcStopTriggerPin;
extern unsigned int 	tdcStartPortOffset;
extern unsigned int 	tdcStopPortOffset;

#endif

extern unsigned short sr_save;
extern unsigned int preemptive_func;
extern unsigned int c_asm_bridge;

volatile void context_switch(taskHandel taskHandel, unsigned char fromISR)
{


	unsigned char makeTheSwitch = 0;
	//If already pre-empted, check prio
	if(osParams.pPreemptingTasks)
	{
		//Check prio
		if((osParams.pPreemptingTasks->pTask->taskType & 0xE0) < (taskHandel->taskType & 0xE0))
			makeTheSwitch = 1;

	}
	else
	{
		makeTheSwitch = 1;
	}

	if(makeTheSwitch)
	{
		//Set preemptive running
		taskHandel->taskType |= 0x80;

		taskList_t switchingTask = malloc(sizeof(struct taskList));

		if(switchingTask)
		{

			switchingTask->pTask = taskHandel;
			switchingTask->pNext = osParams.pPreemptingTasks;
			osParams.pPreemptingTasks = switchingTask;

			preemptive_func = (unsigned int) (taskHandel->func);



			if(fromISR)
			{
				/*
				 *  SP manipulation (with current compiler opt. level)
				 * 	|R9|R10|RET|R11|R12|R13|R14|R15|SR|PCret| -> (2xDDEC) -> |X|X|R9|R10|R11|R12|R13|R14|R15|SR|PCret|
				 * 	Move and Insert
				 *  |R9|R10|RET|R11|R12|R13|R14|R15|SR|PCpreemptive|PCcontexreturn|PCret|
				 */

				asm volatile("	DECD.W SP\r\n" //For RETI
							"	DECD.W SP\r\n" //For
							//"	DECD.W SP\r\n" //For
							//  Move  From					To
							//"	MOV 0x0006(SP), 		0x0002(SP)\r\n"
							//"	MOV 0x0004(SP),			0x0000(SP)\r\n"
							"	MOV 0x0004(SP),			0x0000(SP)\r\n"
							"	MOV 0x0006(SP),			0x0002(SP)\r\n"
							"	MOV 0x0008(SP),			0x0004(SP)\r\n"
							"	MOV 0x000A(SP),			0x0006(SP)\r\n"
							"	MOV 0x000C(SP),			0x0008(SP)\r\n"
							"	MOV 0x000E(SP),			0x000A(SP)\r\n"
							"	MOV 0x0010(SP),			0x000C(SP)\r\n"
							"	MOV 0x0012(SP),			0x000E(SP)\r\n"
							"	MOV 0x0014(SP),			0x0010(SP)\r\n"
							"	MOV 0x0014(SP), 		sr_save\r\n"	//Save final return SR 0x0008(SP),
							"	MOV &preemptive_func, 	0x0012(SP)\r\n"//4
							"	MOV #context_return, 	0x0014(SP)\r\n"//6


				);

				//As we do not
				//__bis_SR_register(GIE);

			}
			else
			{
				/*
				 *  SP manipulation (with current compiler opt. level)
				 * 	|R9|R10|SR|PCret| -> (2xDDEC) -> |X|X|R9|R10|SR|PCret|
				 * 	Move and Insert
				 *  |R9|R10|SR|PCpreemptive|PCcontexreturn|PCret|
				 */
				asm volatile("	DECD.W SP\r\n" //For RETI
							"	DECD.W SP\r\n" //For
							//"	DECD.W SP\r\n" //For
							//  Move  From					To
							//"	MOV 0x0006(SP), 		0x0002(SP)\r\n"
							//"	MOV 0x0004(SP),			0x0000(SP)\r\n"
							"	MOV 0x0004(SP),			0x0000(SP)\r\n"
							"	MOV 0x0006(SP),			0x0002(SP)\r\n"
							"	MOV 0x0008(SP),			0x0004(SP)\r\n"//
							"	MOV 0x0008(SP), 		sr_save\r\n"	//Save final return SR 0x0006(SP),
							"	MOV &preemptive_func, 	0x0006(SP)\r\n"//4
							"	MOV #context_return, 	0x0008(SP)\r\n"//6

				);
			}
		}
		else
		{
			//Just set flag for scheduler to handel event
			//We need task index..
			unsigned char index = 0;
			taskList_t pElem = osParams.pTaskList;
			while(pElem)
			{
				if(pElem->pTask == osParams.sleepUntilTask)
					break;

				index++;
				pElem = pElem->pNext;
			}
			osParams.tasksReady |= (1 << index);
		}

	}
	else
	{
		//Just set flag for scheduler to handel event
		//We need task index..
		unsigned char index = 0;
		taskList_t pElem = osParams.pTaskList;
		while(pElem)
		{
			if(pElem->pTask == osParams.sleepUntilTask)
				break;

			index++;
			pElem = pElem->pNext;
		}
		osParams.tasksReady |= (1 << index);
	}


}


void os_nextPeriodicTask(taskStruct_t taskToRun)
{
	//Find next periodic task to run
	*(taskToRun->pNextRunEarliest) += taskToRun->taskPeriod;

	//Check for a closer task
	taskList_t pElem = osParams.pTaskList;
	while(pElem)
	{
		if(*(pElem->pTask->pNextRunEarliest) < *(taskToRun->pNextRunEarliest))
		{
			if(osParams.runningTask != pElem->pTask)
				if(osParams.time <= *(pElem->pTask->pNextRunEarliest))
					break;
		}

		pElem = pElem->pNext;
	}

	//We found a closer task
	if(pElem)
	{
		//Replace pointer to reuse code below
		taskToRun = pElem->pTask;
	}

	//Update os sleep if this is closer then current TODO: Look over this ones again, seems like this can be made "less"
	if((osParams.time >= osParams.sleepUntil) || (osParams.sleepUntil == 0))
	{
		osParams.sleepUntil = *(taskToRun->pNextRunEarliest);
		osParams.sleepUntilTask = taskToRun;
	}
	else if(*(taskToRun->pNextRunEarliest) <= osParams.sleepUntil)
	{
		osParams.sleepUntil = *(taskToRun->pNextRunEarliest);
		osParams.sleepUntilTask = taskToRun;
	}

}


volatile void context_return()
{
	taskList_t tmp = osParams.pPreemptingTasks;

	//Set preemptive done
	tmp->pTask->taskType &= ~0x80;

	//If timed task ...
	if((tmp->pTask->taskType & 0x0F) == PERIODIC)
	{
		os_nextPeriodicTask(tmp->pTask);
	}


	//Remove head
	osParams.pPreemptingTasks = tmp->pNext;
	free(tmp);

	//Return
	__bis_SR_register(sr_save);

}

unsigned char os_deleteTask(taskHandel element)
{

	taskList_t pCurrentElem = osParams.pTaskList;

	//Save element in front of current element (same as current on first run)
	taskList_t pLastElem = pCurrentElem;

	unsigned short taskIndex = 0;

	while(!(pCurrentElem->pTask == NULL))
	{

			if(pCurrentElem->pTask == element)
			{

				//Rebuild tasksReady vector
				int i;
				unsigned char mask = 0;
				for(i = 0; i < taskIndex-1; i++)
				{
					mask |= (1 << i);
				}
				//TODO: Check if this ends up being correct!
				osParams.tasksReady = (osParams.tasksReady >> 1) + (osParams.tasksReady & mask);
				//Same goes for preemptive list!
				osParams.preemptiveTask = (osParams.preemptiveTask >> 1) + (osParams.preemptiveTask & mask);


				//Remove from list
				if(pCurrentElem == pLastElem)
					osParams.pTaskList = pCurrentElem->pNext;
				else
					pLastElem = pCurrentElem->pNext;


				//Free memory
				free(pCurrentElem->pTask);
				free(pCurrentElem);

				osParams.numberOfTasks--;

				//Task was deleted, return true
				return 1;

			}

			//Move to next list item
			pLastElem = pCurrentElem;
			pCurrentElem= pCurrentElem->pNext;
			taskIndex++;

	}

	///No task was deleted
	return 0;
}

/** Breif: Checks for next task to run
*
*	Goes over the existing tasks status and changes waiting tasks status to ready if
*	ready. While going over the list, the next ready task to run is selected from priority
*	(Higher priority -> Closer to list head).
*
* Param[in/out] taskToRun
*	Pointer to the next task to run, return NULL if notask to run
*
* Return/
* 	none
*/
taskStruct_t os_checkForTask()
{
	taskStruct_t taskToRun = NULL;

	// Pointer to the task list head
	taskList_t pListElem = osParams.pTaskList;
	/* Current task index
	* BIT8	: Flag, next task found
	* BIT4-7: Priority of the found task
	* BIT0-3: Index
	* TODO: Check if this impact performance to much due to masking! The idea of doing it this way
	* is to use less registers and reduce the stack size
	*/
	unsigned char taskIndex = 0;
	unsigned char taskToRunIndex = 0;

	// If no head is found there is nothing to do (obviously this should never be the case)
	if(pListElem->pTask)
	{
		// For as many times as there is tasks ... TODO: Possible overflow problem if maximum amount of tasks is created
		for(taskIndex = 0; (taskIndex & 0x0F) < osParams.numberOfTasks; taskIndex++)
		{

			/*We have a list head, check if task is still ready since last time we checked
			 *If not check if it should be made ready this run (if already ready we save time on checking)
			  */

			if(!(osParams.tasksReady & (1 << ((taskIndex & 0x0F)))))
			{
				// We only need to check the priodic task, as the event tasks is set ready by external events!
				if(*(pListElem->pTask->pNextRunEarliest) <= osParams.time)
					osParams.tasksReady |= (1 << ((taskIndex & 0x0F)));
			}


			if(!(taskIndex & 0x80) && (osParams.tasksReady & (1 << (((taskIndex & 0x0F)))))) 		// If we are not sure this is the task to run
			{							// (due to shared priority) i.e. the "found" flag is not set.

				/*As we are sorted in priority we don't need to keep checking
				 *for it if we have moved further on and no tasks shares priority
				 */

				// If first ready task in list
				if(!taskToRun)
				{
					taskToRun = pListElem->pTask;
					// Save the priority of the task to compare with
					taskIndex |= (pListElem->pTask->taskType & 0xE0);

					taskToRunIndex = taskIndex & 0x0F;

				}

				else
				{
					// Check priority of tasks
					if((taskIndex & 0xE0) > (pListElem->pTask->taskType & 0xE0))
					{
						//The task we have is the one with highest priority
						taskIndex |= 0x80;

					}
					else // If not greater than it must be the same priority (as the list is sorted)
					{
						/*We then prioritize CRITICAL EVENT > EVENT > PERIODIC
						 * and PERIODIC by period
						 */

						// Type of the current selected task
						unsigned short nextTaskType = (taskToRun->taskType & 0x0F);

						// If type is greater (or equal if PERIODIC type) then the other task, it has more priority
						if(nextTaskType >= ((pListElem->pTask->taskType & 0xE0) >> 4))
						{
							//If PERIODIC we choose from lowest period time
							if((nextTaskType >> 4) == PERIODIC)
							{
								// If current period is larger we change task, else we have found the next task
								if(taskToRun->taskPeriod > pListElem->pTask->taskPeriod)
								{
									taskToRunIndex = taskIndex & 0x0F;
									taskToRun = pListElem->pTask;
									// Save the priority of the task to compare with
									taskIndex = (taskIndex & 0x8F) | (pListElem->pTask->taskType & 0xE0);
									// We need to go one more round to be sure this is the task
								}
								else
								{
									//The task we have is the one with highest priority
									taskIndex |= 0x80;


								}
							}

						}
						else // The other task has higher priority
						{
							taskToRun = pListElem->pTask;
							// Save the priority of the task to compare with
							taskIndex = (taskIndex & 0x8F) | (pListElem->pTask->taskType & 0xE0);
							// We need to go one more round to be sure this is the task

						}
					}
				}
			}




			//Iterate to next list element
			if(pListElem->pNext)
				pListElem = pListElem->pNext;

		}
	}

	// Reset ready flag here before starting the task. Avvoiding having to store the index/look for the index after task run
	osParams.tasksReady &= ~(1 << ((taskToRunIndex & 0x0F)));
	return taskToRun;
}


void os_runTask(taskStruct_t taskToRun)
{


	// If periodic, set next periodic wake up
	if((taskToRun->taskType & 0x0F) == PERIODIC)
	{
		os_nextPeriodicTask(taskToRun);
	}

	osParams.runningTask = taskToRun;

	// Run the task
	(taskToRun->func)();

	osParams.runningTask = NULL;

}


int os_schedule()
{
	while(1)
	{
		__bic_SR_register(GIE);

		// Next task to be run from RL
		taskStruct_t taskToRun = NULL;

		// Check for a task to run
		taskToRun = os_checkForTask();

		__bis_SR_register(GIE);

		//Sleep, run, whatever else here
		if(taskToRun == NULL)
			//Sleep
			_BIS_SR(LPM3_bits + GIE);
		else
		{
			os_runTask(taskToRun);
		}
	}

}

//clock_sourc = 0 if internal DCO is uced, 1 if external crystal
//If internal is used its 16Mhz atm soo we don't have to trick with the timer
int os_init(unsigned char clock_source)
{

	//Setup MSP clocks
    //Init MSP430 clocks
	if(!clock_source)
	{
	    BCSCTL1 = CALBC1_16MHZ;
	    DCOCTL = CALDCO_16MHZ;
	    BCSCTL2 = DIVS_3;               // MCLK = DCO, SMCLK =  DCO/8;
	    BCSCTL3 = LFXT1S_0;
	}


	//Initilize values for os structure
	osParams.numberOfTasks = 0;
	osParams.time = 0;
	osParams.timeResolution = 12; //~1ms
	osParams.sleepUntil = 0;
	osParams.tasksReady = 0;
	osParams.preemptiveTask = 0;
	osParams.pPreemptingTasks = NULL;


	//Create list tail for tcb list (dummy node, keep or remove?)
	taskList_t pNewTail = (taskList_t) malloc(sizeof(struct taskList));

	if(pNewTail == NULL)
		//If we are unable to allocate memory return error
		return MEMALOCFAIL;
	else
	{
		pNewTail->pTask = NULL;
		pNewTail->pNext = NULL;

		osParams.pTaskList = pNewTail;
	}

	return 0;
}

void os_start()
{
	//Setup os timer for time controll 32.768kHz cont.
    TA0CTL=TASSEL_1 + MC_2 + ID_0;
    TA0CCR0=33; // Set TACCR0 = 2000 to generate a 1ms timebase @ 16MHz with a divisor of 8
    TA0CCTL0=BIT4 | XCAP_3; // Enable interrupts when TAR = TACCR0

    //Enable global interrupt
    __bis_SR_register(GIE);

    //Start scheduler
    os_schedule();

}


//TODO: Check if code can be compressed
taskHandel os_createTask(void (*func)() ,unsigned char type, unsigned char prio, unsigned char preemptive, unsigned int period)
{

		taskStruct_t pNewTask = (taskStruct_t) malloc(sizeof(struct taskStruct));

		unsigned char taskIndex = 0;

		if(pNewTask == NULL)
		{
			//If we are unable to allocate memory, return error
			return MEMALOCFAIL;
		}
		else
		{

			//Set premetive flag
			osParams.preemptiveTask |= (preemptive << osParams.numberOfTasks);
			//Increment amount of task counter
			osParams.numberOfTasks++;


			//Set task parameters
			pNewTask->func = func;
			pNewTask->taskPeriod = period;
			pNewTask->taskType = ((prio << 4) + (unsigned short) type);


			//Schedule first run (only needed for periodic task)
			if(type == PERIODIC)
			{
				pNewTask->pNextRunEarliest =  malloc(sizeof(long long)); //Saves memory for tasks that are not periodic.
				*(pNewTask->pNextRunEarliest) = osParams.time+pNewTask->taskPeriod; //ev + period hur man vill se det

				//Update os sleep if this is closer then current TODO: Look over this ones again, seems like this can be made "less"
				if((osParams.time >= osParams.sleepUntil) || (osParams.sleepUntil == 0))
					osParams.sleepUntil = *(pNewTask->pNextRunEarliest);
				else if(*(pNewTask->pNextRunEarliest) <= osParams.sleepUntil)
					osParams.sleepUntil = *(pNewTask->pNextRunEarliest);

			}
			else
				pNewTask->pNextRunEarliest = NULL;

			//Allocate for new list element
			taskList_t pNewListElem = (taskList_t) malloc(sizeof(struct taskList));

			if(pNewListElem == NULL)
			{
				//If we are unable to allocate memory, free previusly allocated tcb and return error
				free(pNewTask);
				return MEMALOCFAIL;
			}
			else
			{
				pNewListElem->pTask = pNewTask;

				//Insert in list structure sorted
				taskList_t pCurrentElem = osParams.pTaskList;
				taskList_t pLastElem = osParams.pTaskList;

				while(!(pCurrentElem->pTask == NULL))
				{
					//If new taskhas higher prio, insert infront of current object
					if((pNewListElem->pTask->taskType & 0xE0) >= (pCurrentElem->pTask->taskType & 0xE0))
					{
						pNewListElem->pNext = pCurrentElem;
						if(pCurrentElem == pLastElem)
							osParams.pTaskList = pNewListElem;
						else
							pLastElem->pNext = pNewListElem;

						break;
					}

					//Move on
					pLastElem = pCurrentElem;
					pCurrentElem = pCurrentElem->pNext;
					taskIndex++;

				}


				//If we reached the end of the list insert the object there
				if(pCurrentElem->pTask == NULL)
				{
					pNewListElem->pNext = pCurrentElem;
					if(pCurrentElem == pLastElem)
						osParams.pTaskList = pNewListElem;
					else
						pLastElem->pNext = pNewListElem;
				}

			}

		}

		//Rebuild preemptive vector
		int i;
		unsigned char mask = 0;
		for(i = 0; i < taskIndex-1; i++)
		{
			mask |= (1 << i);
		}
		osParams.preemptiveTask = (osParams.preemptiveTask >> 1) + (osParams.preemptiveTask & mask);

		//Return 0 as in 0k
		return pNewTask;


}

unsigned char os_postEvent(taskHandel taskHandel)
{

	//Check that the task is an event task
	if((taskHandel->taskType & 0x0F) == EVENT)
	{
		//Find task index
		taskList_t pListElem = osParams.pTaskList;
		int taskIndex = 0;
		while(!pListElem->pTask == NULL)
		{
			if(pListElem->pTask == taskHandel)
				break;

			pListElem = pListElem->pNext;
			taskIndex++;
		}

		//If the task is preemptive and not already running,run it now!
		if((osParams.preemptiveTask & (1 << taskIndex)) && !(taskHandel->taskType & 0x80))
		{
			context_switch(taskHandel, 0);
		}
		else
		{
			//Just set flag for scheduler to handel event
			//osParams.tasksReady |= (1 << taskIndex);
		}

	}

	return 0;
}


#ifdef PWMMODULE

//Set PWM frequency (0-255Hz) and start PWM on Timer A1
void os_pwmInit(unsigned char freq)
{

	//Check if timer is running
	unsigned int running = TA1CCR0;

	//Calculate and set timer scale factor
	unsigned int freqScale = 32768 / freq;

	TA1CCR0 = freqScale;

	//If PWM is not currently running, start it.
	if(!running)
	{
		TA1CCR1 = 0;
		TA1CCR2 = 0;

		TA1CCTL1 = OUTMOD_2;
		TA1CCTL2 = OUTMOD_2;

		TA1CTL=TASSEL_1 + MC_1 + ID_0;
	}
	//Else if running, rescale duty cycles
	else
	{
		//How to?
		unsigned char dutyCycle1 = 0;
		unsigned char dutyCycle2 = 0;

		TA1CCTL1 = dutyCycle1;
		TA1CCTL2 = dutyCycle2;
	}

}

//Start/Set PWM on a pin
void os_pwmSetPWM(unsigned char pin, unsigned char dutyCycle)
{
	//Check if PWM is running
	unsigned int running = TA1CCR0;

	//If running we continue
	if(running > 0)
	{
		//Check the dutyCycle limits
		if(dutyCycle > 100)
			dutyCycle = 100;

		//Calculate scaling factor
		unsigned int scaleFactor = running*(dutyCycle/100);

		//What is the pin selection?
		ss

	}
}

//Stop PWM on a pin (sets the pin as GPIO)
void os_pwmStopPWM(unsigned char pin)
{
	P2SEL &= ~pin;
}


#endif

// "TDC module" implementation
#ifdef TDCMODULE

void os_tdcInit(unsigned char startTrigger, unsigned char startTriggerEdge, unsigned int startPort, unsigned char stopTrigger, unsigned char stopTriggerEdge, unsigned int stopPort, unsigned char resolution)
{
	__disable_interrupt();

    tdcStartTriggerPin				= startTrigger;
    tdcStopTriggerPin 				= stopTrigger;
    osParams.tdcStartTriggerEdge 	= startTriggerEdge;
    osParams.tdcStopTriggerEdge 	= stopTriggerEdge;
    tdcStartPortOffset				= startPort + 0x0003; 	//Add offset to point to PxIFG
    tdcStopPortOffset				= stopPort + 0x0003; 	//Add offset to point to PxIFG

    osParams.tdcValue = 0;

    __enable_interrupt();

}


unsigned int os_tdcRun()
{
	__disable_interrupt();
	osParams.tdcRunning = 0b01;

	//Arm start trigger
    asm(
    		" PUSH R10\r\n"										 //Save R10
    		" MOV  &tdcStartPortOffset,	R10\r\n"				 //Put in R10 for adressing magic
    		" BIC  &tdcStartTriggerPin,  2(R10)\r\n"			 //Clear IE
    	);
    	//Check if R or F edge
		if(osParams.tdcStartTriggerEdge) 			//R
			asm(" BIS &tdcStartTriggerPin, 1(R10)");
		else										//F
			asm(" BIC &tdcStartTriggerPin, 1(R10)");

    asm(
    		" BIC.B &tdcStartTriggerPin, 0(R10)\r\n"			//Clear IFG flag
    		" BIS  &tdcStartTriggerPin,  2(R10)\r\n"			//Set IE
    		" POP R10\r\n"										//Restore R10
    	);

	__enable_interrupt();

	while(osParams.tdcRunning == 0b10){}

	while(!(osParams.tdcRunning == 0b11)){}

	//Tdc value ready
	osParams.tdcRunning = 0b00;

	return osParams.tdcValue;



}

void tdcISR(unsigned char IFG)
{

	if((IFG & tdcStartTriggerPin) || (IFG & tdcStopTriggerPin))
	{
		//Start trigger
		if(osParams.tdcRunning == 0b01)
		{

			//Sample timer
			osParams.tdcValue = TA0R;

			c_asm_bridge = osParams.tdcStopTriggerEdge;

			//Change to stop pin
		    asm(
		    		" PUSH R10\r\n"										 //Save R10
		    		" MOV  &tdcStartPortOffset,	R10\r\n"				 //Put in R10 for adressing magic
		    		" BIC  &tdcStartTriggerPin,  2(R10)\r\n"			 //Clear IE
		    		" MOV  &tdcStopPortOffset,	R10\r\n"				 //Put in R10 for adressing magic

		    		" BIT #1, &c_asm_bridge\r\n"
		    		" JNZ (FEDGE)\r\n"
		    		" BIC &tdcStopTriggerPin, 1(R10)\r\n"
		    		" JMP (EDGESET)\r\n"
		    		"FEDGE:\r\n"
		    		" BIS &tdcStopTriggerPin, 1(R10)\r\n"
		    		"EDGESET:\r\n"
					" BIS  &tdcStopTriggerPin,  2(R10)\r\n"			 	//Set IE
					" POP R10\r\n"										//Restore R10
		    	);


			osParams.tdcRunning = 0b10;


		}
		//Stop trigger
		else if(osParams.tdcRunning == 0b10)
		{

			unsigned int tmp;
			// Stop tdc
			tmp = TA0R;

			if(tmp < osParams.tdcValue)
			{
				osParams.tdcValue = (65535 - osParams.tdcValue) + tmp;
			}
			else
				osParams.tdcValue = tmp - osParams.tdcValue;

			//disable
		    asm(
		    		" PUSH R10\r\n"										 //Save R10
		    		" MOV  &tdcStopPortOffset,	R10\r\n"				 //Put in R10 for adressing magic
		    		" BIC  &tdcStopTriggerPin,  2(R10)\r\n"			 	//Clear IE
		    		" POP R10\r\n"
		    	);
			//
			osParams.tdcRunning = 0b11;

		}


	}

}

#endif



//TODO: Wake uo on event from interrupts
#pragma vector=TIMER0_A0_VECTOR
__interrupt  void timer0A0ISR(void)
{
// Timer A0 Interrupt service routine

	if(1)//TODO: Implement SW for overflow correction
	{
		TA0CCR0+=33;
		//Count one ms
		osParams.time += 1;

		//TACCR0 += osParams.timeResolution;

		if((osParams.time==osParams.sleepUntil))
		{
			//Exit LPM incase we are sleeping
			_BIC_SR(LPM3_EXIT);

			//We need task index..
			unsigned char index = 0;
			taskList_t pElem = osParams.pTaskList;
			while(pElem)
			{
				if(pElem->pTask == osParams.sleepUntilTask)
					break;

				index++;
				pElem = pElem->pNext;
			}

			if((osParams.preemptiveTask & (1 << index)) && osParams.runningTask)
			{

				//Might be smart to make a "buffer" function to run CS after ISR is over.
				context_switch(osParams.sleepUntilTask,1);
			}
			else
			{


				//Lets assume its correct to save the check
				//osParams.tasksReady |= (1 << index);
			}
		}
	}
	else
	{
		//Correct time at overflow, correction 1023 - 0, we want to round to closest 1000. so > 500 = 1000, else 0
		unsigned int correction = osParams.time & 0x03FF;
		if(correction > 500)
			correction = 1000;
		else
			correction = 0;

		osParams.time &= ~correction;
		osParams.time|= correction;

	}
}

//
////All interrupts are handled by the OS, user binds callbacks to the specific interrupts

#pragma vector=PORT1_VECTOR
__interrupt  void port1ISR(void)
{

#ifdef TDCMODULE
	tdcISR(P1IFG);
#endif

	//Clear all flags
	P1IFG = 0x00;
}

#pragma vector=PORT2_VECTOR
__interrupt  void port2ISR(void)
{

#ifdef TDCMODULE
	tdcISR(P2IFG);
#endif


	//Clear all flags
	P2IFG = 0x00;
}

