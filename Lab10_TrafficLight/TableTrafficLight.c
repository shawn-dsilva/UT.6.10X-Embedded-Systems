// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"

#define LIGHT                   (*((volatile unsigned long *)0x400050FC))
#define SWITCH                  (*((volatile unsigned long *)0x4002401C))
#define WALK										(*((volatile unsigned long *)0x40025028))
#define NVIC_ST_CTRL_R      		(*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R    		(*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R   		(*((volatile unsigned long *)0xE000E018))

// ***** 2. Global Declarations Section *****
// FUNCTION PROTOTYPES: Each subroutine defined
void Port_Init(void);
void SysTick_Init(void);
void SysTick_Wait(unsigned long delay);
void SysTick_Wait10ms(unsigned long delay);
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

// ***** 3. Subroutines Section *****

//******* SysTick functions follow ********
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}
// The delay parameter is in units of the 80 MHz core clock. (12.5 ns)
void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}
// 10000us equals 10ms
void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(800000);  // wait 10ms
  }
}
//******** SysTick functions end **********

//Initializes Ports B,E and F
void Port_Init(void){ 										
	volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x32;     // B E F clock
  delay = SYSCTL_RCGC2_R; 	// delay 

  //PORT_F INIT
	GPIO_PORTF_PCTL_R = 0x00;       //  PCTL GPIO on PF3, PF1
  GPIO_PORTF_DIR_R |= 0x0A;       //  PF3, PF1 are outputs
  GPIO_PORTF_AFSEL_R = 0x00;      //  disable alternate function
  GPIO_PORTF_PUR_R = 0x00;        // disable pull-up resistor
  GPIO_PORTF_DEN_R |= 0x0A;       //  enable digital I/O on PF3, PF1
	
	//PORT_B INIT
	GPIO_PORTB_AMSEL_R &= ~0x3F; // disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; //  enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;    // outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F; // regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;    // enable digital on PB5-0	
	
	//PORT_E INIT
	GPIO_PORTE_AMSEL_R &= ~0x07; // disable analog function on PE1-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // enable regular GPIO
  GPIO_PORTE_DIR_R &= ~0x07;   //  inputs on PE1-0
  GPIO_PORTE_AFSEL_R &= ~0x07; //  regular function on PE1-0
  GPIO_PORTE_DEN_R |= 0x07;    //  enable digital on PE1-2
}



// *********** 4. Data Structures *************

//FSM structure declaration
struct State {
		unsigned long SignalLed;
		unsigned long WalkLed;
		unsigned long Time;
		unsigned long Next[8];
	};
typedef const struct State STyp;
	
//State Numbers defined with keywords
#define GO_SOUTH         0 //0x21 West/East Red,North/South Green 		 ||  RG  ||  "GoN"
#define WAIT_SOUTH       1 //0x22 West/East Red,North/South Yellow 	   ||  RY  ||  "WaitN"
#define GO_WEST          2 //0x0C West/East Green,North/South Red 		 ||  GR  ||  "GoE"
#define WAIT_WEST        3 //0x14 West/East Yellow,North/South Red  	 ||  YR  ||  "WaitE"
#define WALK_GREEN       4 //Green LED walk state
#define WALK_RED         5 //Red LED stop state
#define WALK_OFF         6 //Red LED off state
#define WALK_RED_2       7
#define WALK_OFF_2       8

//Walk LED values defined by keywords
#define OFF 0
#define RED 2
#define GREEN 8


//FSM State Transition Graph Structure
STyp FSM[9]={

 {0x21, RED, 200, {GO_SOUTH,  WAIT_SOUTH,  GO_SOUTH,  WAIT_SOUTH,  WAIT_SOUTH,  WAIT_SOUTH,  WAIT_SOUTH,  WAIT_SOUTH}}, 
 
 {0x22, RED, 100, {GO_WEST,  GO_WEST,  GO_SOUTH,  GO_WEST,  WALK_OFF,  GO_WEST,  WALK_GREEN,  GO_WEST}},
 
 {0x0C, RED, 200, {WAIT_WEST,  GO_WEST,  WAIT_WEST,  WAIT_WEST,  WAIT_WEST,  WAIT_WEST,  WAIT_WEST,  WAIT_WEST}},

 {0x14, RED, 100, {GO_SOUTH,  GO_WEST,  GO_SOUTH,  GO_SOUTH,  WALK_OFF,  WALK_GREEN,  GO_SOUTH,  WALK_GREEN}},
 
 {0x24, GREEN, 200, {WALK_RED_2,  WALK_RED_2,  WALK_RED_2,  WALK_RED_2,  WALK_GREEN,  WALK_RED_2,  WALK_RED_2,  WALK_OFF}}, //0x24 West/East Red,North/South Red 				 ||  RR  ||  
 
 {0x24, RED, 50, {WALK_OFF,  WALK_OFF,  WALK_OFF,  WALK_OFF,  WALK_OFF,  WALK_OFF,  WALK_OFF,  GO_SOUTH}},
 
 {0x24, OFF, 50, {GO_SOUTH,  GO_WEST,  GO_SOUTH,  GO_WEST,  WALK_GREEN,  GO_WEST,  GO_SOUTH,  WALK_RED_2}},
 
 {0x24, RED, 50, {WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2,  WALK_OFF_2}},
 
 {0x24, OFF, 50, {WALK_RED,  WALK_RED,  WALK_RED,  WALK_RED,  WALK_GREEN,  WALK_RED,  WALK_RED,  WALK_RED}}
 
}; 


unsigned long S;  // index to the current state 
unsigned long Input; //Variable for input

//MAIN
int main(void){ 
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
	
  SysTick_Init(); //Timer init
	Port_Init();  //Ports B,E,F init

  S = GO_SOUTH; //Initial state 
  
  EnableInterrupts();
	
	//Main Loop
  while(1){
		LIGHT = FSM[S].SignalLed;  // set Signal Lights
		
		WALK = FSM[S].WalkLed;  //set Walk Lights
		
    SysTick_Wait10ms(FSM[S].Time);// Wait timer
		
    Input = SWITCH;     // read Switch input
		
    S = FSM[S].Next[Input]; // next State based on current input
     
  }
}




