// UARTTestMain.c
// Runs on LM4F120/TM4C123
// Used to test the UART.c driver
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
	
char mode, color1,massage;
int mode1;
int i = 0; //counter for PF0(switch 2)
int xin= 0;
int check = 0x01;

int pcheck;
char lettersend;

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}
//debug code

void UART1_Init(void){
	SYSCTL_RCGCUART_R |= 0x0002;//activate UART1
	SYSCTL_RCGCGPIO_R |= 0x0002;//activate port B
	UART1_CTL_R &= ~0x0001;     //disable UART
	UART1_IBRD_R = 8;//int(16*115200)=int(27.1267)
	UART1_FBRD_R = 44;// round(0.1267 *64) = 8
	UART1_LCRH_R = 0x0070;//8bit length,enable FIFO;
	UART1_CTL_R = 0x0301;//enable RXE,TXE and UART
	GPIO_PORTB_AFSEL_R |= 0x03; // alt function on PB1-0
	GPIO_PORTB_DEN_R |= 0x03;  
	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFFFF00)+0x00000011;
	//UART1: PB0 -> U1Rx,PB1 -> U1tx
  GPIO_PORTB_AMSEL_R &= ~0x03;          // disable analog functionality on PB 0-1
}
void PortE_Init(){
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x10;           // 1) E clock
  delay = SYSCTL_RCGC2_R;           // delay       
  GPIO_PORTE_AMSEL_R &= ~0x01;        // 3) disable analog function
  GPIO_PORTE_PCTL_R &= ~0x0000000F;   // 4) GPIO clear bit PCTL  
  GPIO_PORTE_DIR_R &= ~0x01;          // 5) PE0  input   
  GPIO_PORTE_AFSEL_R &= ~0x01;        // 6) no alternate function       
	GPIO_PORTE_DEN_R |= 0x01;          // 7) enable digital pins PE0
	}

void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  // only PF0 needs to be unlocked, other bits can't be locked
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog on PF
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) disable alt funct on PF7-0
  GPIO_PORTF_PUR_R = 0x11;          // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital I/O on PF4-0
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4,PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4,PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4,PF0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flags 4,0
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4,PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|0x00400000; // (g) bits:23-21 for PORTF, set priority to 2
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC        
}

void colorchange(char color){
	if (color == 'r')
		{
			GPIO_PORTF_DATA_R = 0x02;
		}
	else if (color == 'b')
		{
			GPIO_PORTF_DATA_R = 0x04;
		}
	else if (color == 'g')
		{
			GPIO_PORTF_DATA_R = 0x08;
		}
	else if (color == 'y')
		{
			GPIO_PORTF_DATA_R = 0x0A;
		}
	else if (color == 'w')
		{
			GPIO_PORTF_DATA_R = 0x0E;
		}
	else if (color == 'p')
		{
			GPIO_PORTF_DATA_R = 0x06;
		}
	else{GPIO_PORTF_DATA_R = 0x00;}
}

void menu(){
	UART_OutString("Welcome to CECS 447 Project 2 - UART");
	OutCRLF();
	UART_OutString("Please choose a communication mode(type 1 or 2 or 3)");
	mode = UART_InChar();
	OutCRLF();
	UART_OutChar(mode);
	OutCRLF();
	

}
void modechange(char abc){
	if (abc == '1')
		{
			mode1 = 1;
		}
	else if (abc == '2')
	{
			mode1 = 2;
	}

}
char UART1_Receiver(void){
	char data;
	while(UART1_FR_R !=0);
	data = UART1_DR_R;
	return (unsigned char) data;

}
void	UART1_TRansmitter(unsigned char data){
	while(UART1_FR_R !=0);
	UART1_DR_R = data;

}
void GPIOPortF_Handler(void){


if(GPIO_PORTF_RIS_R&0x01){
		GPIO_PORTF_ICR_R = 0x01;
		if(i < 5){
		i = i + 1;
		}
		else{
		i = 0;
		}
		if(i == 0)
				{GPIO_PORTF_DATA_R = 0x00;
				 UART_OutChar('d');
				 lettersend = 'd';
				}
		else if(i == 1)
				{GPIO_PORTF_DATA_R = 0x02;
				 UART_OutChar('r');
				 lettersend = 'r';
				}		
		else if(i == 2)
				{GPIO_PORTF_DATA_R = 0x08;
				 UART_OutChar('g');
				 lettersend = 'g';
				}
		else if(i == 3)
				{GPIO_PORTF_DATA_R = 0x04;
				 UART_OutChar('b');
				 lettersend = 'b';
				}
		else if(i == 4)
				{GPIO_PORTF_DATA_R = 0x06;
				 UART_OutChar('p');
				 lettersend = 'p';
				}
		else if(i == 5)
				{GPIO_PORTF_DATA_R = 0x0E;
				 UART_OutChar('w');
				 lettersend = 'w';	
				}
		


}
if(	GPIO_PORTF_RIS_R&0x10) 
	{ 												
		GPIO_PORTF_ICR_R |= 0x10;
		//GPIO_PORTF_DATA_R = 0x0E;
		xin = 100;
		
	}
}

int main(void){
  unsigned char i,letterget;
	char string[20];
  //char color[20];  // global to assist in debugging
//	char color1;
  //unsigned long n;
	PortF_Init();
  //PLL_Init();
  UART_Init();              // initialize UART
	UART1_Init();
//  OutCRLF();
//  for(i='A'; i<='Z'; i=i+1){// print the uppercase alphabet
//    UART_OutChar(i);
//  }
//  OutCRLF();
//  UART_OutChar(' ');
//  for(i='a'; i<='z'; i=i+1){// print the lowercase alphabet
//    UART_OutChar(i);
//  }
//  OutCRLF();
//  UART_OutChar('-');
//  UART_OutChar('-');
//  UART_OutChar('>');
//  OutCRLF();
  while(1){
		while(UART1_InChar() == 'm'){
			letterget = UART1_InChar();
			//UART_OutChar(letterget);
				colorchange(letterget);
			while(xin != 100);
				UART1_OutChar('m');
				UART_OutChar('m');
				UART1_OutChar(lettersend);
				UART_OutChar(lettersend);
				xin = 0;
		}
		while(UART1_InChar() == 'n'){	
			//UART_OutChar('y');
			letterget = UART1_InChar();
			UART_OutChar(letterget);
		}
	}
  //}
}
