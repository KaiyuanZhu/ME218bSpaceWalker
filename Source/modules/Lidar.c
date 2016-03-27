#include "ES_Configure.h"
#include "ES_Framework.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_i2c.h"
#include "driverlib/gpio.h"
#include "termio.h"
#include "ES_Port.h"
#include "MotorService.h"
#include "PWM8Tiva.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "Lidar.h"
#include "ES_PostList.h"
#include "MasterSM.h"
#define ALL_BITS (0xff<<2)
#define TicksPerMS 40000
#define PeriodInMS 1
#define BitsPerNibble 4

void Init_LidarCapture(void);
void Response_LidarCapture(void);
bool Check5Keystroke(void);
float QueryDistance(void);




/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static LidarState_t CurrentLidarState;
//static uint32_t data[OneCircleStepNumber];
static float Period;
static float Period2Return;
static float Distance;
static uint32_t ThisCapture;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static bool TriggerLow;

bool InitLidarService( uint8_t Priority )
{
  ES_Event ThisEvent;
  MyPriority = Priority;
	Init_LidarCapture();
	_HW_Timer_Init(ES_Timer_RATE_1mS);
	//ES_Timer_InitTimer(AD_TIMER, 100);
	ThisEvent.EventType = ES_INIT;
	CurrentLidarState = InitLidar;
	 HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
	HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= BIT2HI;
	// Set PE2 as output pin
	HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= BIT2HI;
	// Write 1 to PE2, this will not trigger the measurement
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT2HI; 
	// PE1 to power up the Lidar 3.3V
	HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= BIT1HI;
	HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= BIT1HI;
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT1HI; 
	
	
	
	TriggerLow = false;
	Period = 0;
	printf("Initialization done\n\r");
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

bool PostLidarService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

ES_Event RunLidarService( ES_Event ThisEvent )  
{
//	ES_Event New_Event;
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentLidarState)
  {
    case InitLidar:       // If current state is initial Psedudo State
        if (ThisEvent.EventType == ES_INIT)// only respond to ES_Init
        {
            CurrentLidarState = LidarStart;				
						printf("\r\n Lidar Inited \r\n");	
	
            //ES_Timer_InitTimer(LIDAR_TIMER, 100);			
		
         }
    break;

    case LidarStart: 
			   if (ThisEvent.EventType == GET_DISTANCE)
			  {		//printf("\r\n GET_DISTANCE received in Lidar");
            if (Period == 0){
               HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
							 TriggerLow = true;					
               CurrentLidarState = LidarMeasure;		
							 ES_Timer_InitTimer(LIDAR_TIMER, 50);
							ReturnEvent.EventType = ES_NO_EVENT;
						}							
      			
				}
		break;
				
		case LidarMeasure:
			if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == LIDAR_TIMER)
				{
				 if (Period2Return != 0){
					 
					//Distance = (float)Period2Return/(float)800; 
					 Distance = (float)((Period2Return*25/20000)*2.02-21.87);
					 ES_Event Event2Post;
					 Event2Post.EventType = DISTANCE_FOUND;
					 PostMasterSM(Event2Post);
					  TriggerLow = false;			
					  printf("\r\n Lidar Measure result is %f", Distance);
					  HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT2HI; 
						Period = 0; 
           CurrentLidarState = LidarStart;
           ReturnEvent.EventType = ES_NO_EVENT;					 
					 }						 
				 } else {
					 ES_Timer_InitTimer(LIDAR_TIMER, 50);
					 ReturnEvent.EventType = ES_NO_EVENT;
				 }
		break;
  }                                   // end switch on Current State
  return ReturnEvent;
}

void Init_LidarCapture(void){

  // start by enabling the clock to the timer (Wide Timer 0)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
	// enable the clock to Port C
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
	// since we added this Port C clock init, we can immediately start
  // into configuring the timer, no need for further delay
  
  // make sure that timer (Timer A&B) is disabled before configuring
  //HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;

	// set it up in 32bit wide (individual, not concatenated) mode
	// the constant name derives from the 16/32 bit timer, but this is a 32/64
	// bit timer so we are setting the 32bit mode
  HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

	// we want to use the full 32 bit count, so initialize the Interval Load
	// register to 0xffff.ffff (its default value :-)
 // HWREG(WTIMER0_BASE+TIMER_O_TAILR) = 0xffffffff;
  HWREG(WTIMER0_BASE+TIMER_O_TBILR) = 0xffffffff;

	// set up timer A in capture mode (TAMR=3, TAAMS = 0), 
	// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
//  HWREG(WTIMER0_BASE+TIMER_O_TAMR) = 
//      (HWREG(WTIMER0_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | 
//        (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
				
	// set up timer B in capture mode (TAMR=3, TAAMS = 0), 
	// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER0_BASE+TIMER_O_TBMR) = 
      (HWREG(WTIMER0_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) | 
        (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);
				
	// To set the event to rising edge, we need to modify the TAEVENT/TBEVENT bits 
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT/TBEVENT bits
  //HWREG(WTIMER0_BASE+TIMER_O_CTL) |= TIMER_CTL_TAEVENT_M;
	HWREG(WTIMER0_BASE+TIMER_O_CTL) |= TIMER_CTL_TBEVENT_M;

	// Now Set up the port to do the capture (clock was enabled earlier)
	// start by setting the alternate function for Port C bit 4 (WT0CCP0) for Timer 0A
	// and set the alternate function for Port C bit 5 (WT0CCP1) for Timer 1A
	HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT5HI);

	// Then, map bit 4's alternate function to WT0CCP0
	// 7 is the mux value to select WT0CCP0, 16 to shift it over to the
	// right nibble for bit 4 (4 bits/nibble * 4 bits)
	//HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = 
   // (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xfff0ffff) + (7<<16);

	// Then, map bit 5's alternate function to WT0CCP0
	// 7 is the mux value to select WT0CCP1, 20 to shift it over to the
	// right nibble for bit 5 (4 bits/nibble * 5 bits)
	HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = 
    (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xff00ffff) + (7<<20) + (7<<16);

	// Enable pin on Port C for digital I/O
	HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT5HI);
	
	// make pin 4&5 on Port C into an input
	HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= ~(BIT5HI);

	HWREG(GPIO_PORTC_BASE+GPIO_O_PUR) |= (BIT5HI);
	// back to the timer to enable a local capture interrupt
 // HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
  HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;

	// enable the Timer A&B in Wide Timer 0 interrupt in the NVIC
	// it is interrupt number 94&95 so appears in EN2 at bit 30 & 31
  HWREG(NVIC_EN2) |= (BIT30HI | BIT31HI);

	// make sure interrupts are enabled globally
  __enable_irq();

	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
  //HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN| TIMER_CTL_TASTALL);
  HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN| TIMER_CTL_TBSTALL);
  	
}

 void Response_LidarCapture(void)
{
	HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
	//Clear the source of the interrupt
	 static uint32_t  LastCapture;
	//Grab the capture value and calculate for the period
	
	if (TriggerLow == true){
	ThisCapture = HWREG(WTIMER0_BASE+TIMER_O_TBR);
	Period = ThisCapture - LastCapture;
		if (Period < 310000)
		{
			Period2Return = Period;
		} 
	// update LastCapture to prepare for the next edge
  }
	LastCapture = ThisCapture;
}

float QueryDistance(void)
{
	return Distance;
}
 
