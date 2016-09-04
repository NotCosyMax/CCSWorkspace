#include <msp430g2553.h>
#include "maxos.h"
#include <intrinsics.h>

/* Motor PIN defines */
#define MOTOR_R_0		BIT1
#define MOTOR_R_1		BIT2
#define MOTOR_L_0		BIT4
#define MOTOR_L_1		BIT5



int i;

taskHandel green_led_task = 0;
taskHandel red_led_task = 0;
taskHandel uart_receive_task = 0;
taskHandel uart_transmit_task = 0;

int return_argument = 0;

char to_transmit = 0x00;

unsigned int tmp;

static void uart_transmit()
{

	UCA0TXBUF = to_transmit;



	to_transmit = 0;
}


static void blink_red()
{
	os_tdcInit(BIT0, RISING_EDGE, PORT2, BIT0, FALLING_EDGE, PORT2, ID_3);
	while(1){
		tmp = os_tdcRun();
	}

}


static void blink_green()
{
	P1OUT ^= BIT6;
}




void init_led()
{
	P1DIR = BIT6;
	P1OUT = BIT6;
}

void init_uart()
{
	P1SEL  |=  BIT1 + BIT2;  // P1.1 UCA0RXD input
	P1SEL2 |=  BIT1 + BIT2;  // P1.2 UCA0TXD output

	 UCA0CTL1 |=  UCSSEL_1 + UCSWRST;  // USCI Clock = SMCLK,USCI_A0 disabled
	   UCA0BR0   =  3;                 // 104 From datasheet table-
	   UCA0BR1   =  0;                   // -selects baudrate =9600,clk = SMCLK
	   UCA0MCTL  =  UCBRS_3;             // Modulation value = 1 from datasheet
	   //UCA0STAT |=  UCLISTEN;            // loop back mode enabled
	   UCA0CTL1 &= ~UCSWRST;             // Clear UCSWRST to enable USCI_A0

	   //IE2 |= UCA0TXIE;                  // Enable the Transmit interrupt
	   IE2 |= UCA0RXIE;                  // Enable the Receive  interrupt
}

static void uart_receive()
{
	to_transmit = UCA0RXBUF;
	os_postEvent(uart_transmit_task);
}



//Varaible sharing test Gloabl asm -> C

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer


    return_argument = os_init(0);

    while(return_argument < 0)
    {
    	//Something is wrong, check error code
    }

    init_led();
    init_uart();
    red_led_task = os_createTask(&blink_red,PERIODIC,1,1,500);

	P2SEL |= BIT1 + BIT4;
	P2DIR |= BIT1 + BIT4;


    //Start PWM
    os_pwmInit(100);
    os_pwmSetPWM(pin,50);

    while(red_led_task < 0)
    {
    	//Something is wrong, check error code
    }

    green_led_task = os_createTask(&blink_green,PERIODIC,1,1,10);


    while(green_led_task < 0)
    {
    	//Something is wrong, check error code
    }
/*
    uart_receive_task = os_create_task(&uart_receive,CRITICAL_EVENT,1,0);

    while(uart_receive_task < 0)
    {
    	//Something is wrong, check error code
    }

    uart_transmit_task = os_create_task(&uart_transmit,EVENT,1,0);

    while(uart_transmit_task < 0)
    {
    	//Something is wrong, check error code
    }
*/


    //

    //Set pin

    P2SEL |= MOTOR_R_0;

    os_start();

}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void ReceiveInterrupt(void)
{
  os_postEvent(uart_receive_task);
  IFG2 &= ~UCA0RXIFG; // Clear RX flag
}


