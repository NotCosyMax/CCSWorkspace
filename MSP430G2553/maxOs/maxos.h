/*
 * maxos.h
 *
 *  Created on: 13 mar 2016
 *      Author: maxwe
 */

#ifndef MAXOS_OLD_MAXOS_H_
#define MAXOS_OLD_MAXOS_H_

//Task types
#define SINGLE	 		0x0		//Runs ones and are then released
#define PERIODIC 		0x1		//Runs periodicly
#define	EVENT	 		0x2		//Runs on triggered event

#ifdef TDCMODULE

#define RISING_EDGE		0x00
#define FALLING_EDGE	0x01

#define PORT1			0x0020
#define PORT2			0x0028
#endif

//Error codes
#define MEMALOCFAIL		-1				//Memory allocation failed
#define NOTASKCREATED	-2				//Task where not created

//Structure typedefs
typedef struct taskStruct* 	taskStruct_t;
typedef struct taskStruct* 	taskHandel;

//Function declaration
int 				os_init(unsigned char clock_source);
void 				os_start();
unsigned char 		os_postEvent(taskHandel taskHandel);
taskHandel 			os_createTask(void (*func)() ,unsigned char type, unsigned char prio, unsigned char preemptive, unsigned int period);
unsigned char 		os_remove_tcb_from_list(taskHandel element);

#ifdef TDCMODULE

void 				os_tdcInit(unsigned char startTrigger, unsigned char startTriggerEdge, unsigned int startPort, unsigned char stopTrigger, unsigned char stopTriggerEdge, unsigned int stopPort, unsigned char resolution);
unsigned int 		os_tdcRun();

#endif


#ifdef PWMMODULE
void 				os_pwmInit(unsigned int freq);
void 				os_pwmSetPWM(unsigned char pin, unsigned char dutyCycle);
void				os_pwmStopPWM(unsigned char pin);

#endif

//macros
#define os_remove_task(i,t)	os_remove_tcb_from_list(i,t);


struct taskStruct {

	//Task function pointer
	void 				(*func)();

	//Task period (no gain in making this allcocated only for period tasks as the pointer remains the same size)
	unsigned int 		taskPeriod;

	//Task type: PERIODIC, EVENT, MORE TO ADD (Type in BIT3-0, prio in BIT6-4, preempted status in BIT7)
	unsigned char		taskType;

	//Next run time for task (pointer to save 6B RAM for non periodic tasks)
	unsigned long long*	 pNextRunEarliest;

	//Task lateness
	//int					taskLateness;

};




#endif /* MAXOS_OLD_MAXOS_H_ */
