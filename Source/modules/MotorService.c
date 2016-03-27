/****************************************************************************
 Module
   HSMTemplate.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.


/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/gpio.h"
#include "termio.h"
#include "ES_Port.h"
#include "PWM8Tiva.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "MotorService.h"
#include "RPMControl.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines
#define ALL_BITS (0xff<<2)
#define PWMTicksPerMS 40000
#define PeriodInMS 1
#define ONE_SEC  976
#define BitsPerNibble 4
#define OneCircleStepNumber 72	// number of steps in one circle rotation
#define MsPerDegree 50			// for rotating task
#define MsPerCm 20
#define RPM_DC_RATIO 0.25
#define REV 0
#define FOR 1

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
int16_t GetTargetRPMLeft(void);
int16_t GetTargetRPMRight(void);
LastEvent_t GetLastEvent(void);
static void InitPWM(void);
static void PWMUpdateMotorLeft(int32_t);
static void PWMUpdateMotorRight(int32_t);
void Stop(void);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
//static uint16_t distance;
//static uint16_t angle;
static LastEvent_t lastevent;
static int16_t LeftDutyCycle;
static int16_t RightDutyCycle;
static int16_t LeftTargetRPM;
static int16_t RightTargetRPM;
static ControlState SpeedControlState;
static float distance;
static float AngleToTurn;
static float lastAngle;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunTemplateSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/

ES_Event RunMotorService( ES_Event CurrentEvent )
{
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
            switch (CurrentEvent.EventType)
            {
               case FORWARD :
									InitPIControl();								 
									LeftDutyCycle = 50;
									RightDutyCycle = 50;
							 		if(CurrentEvent.EventParam < 1){
										distance = 20.0;
									}else{
										distance = CurrentEvent.EventParam*1.0;
									}	
									//distance = CurrentEvent.EventParam*1.0;
									printf("\r\n distance get from event parm%f", distance);
									PWMUpdateMotorRight(RightDutyCycle);
							    PWMUpdateMotorLeft(LeftDutyCycle);
									printf("\r\n forward");
                  ReturnEvent.EventType = ES_NO_EVENT;
                  lastevent = forward;
							 		LeftTargetRPM = (int16_t)(LeftDutyCycle*RPM_DC_RATIO);
						      RightTargetRPM = (int16_t)(RightDutyCycle*RPM_DC_RATIO);	
                  break;
							 
//						case BACKWARD :
//									InitPIControl();								 
//									LeftDutyCycle = -40;
//									RightDutyCycle = -40;
//									distance = CurrentEvent.EventParam*1.0;
//									printf("\r\n distance get from event parm%f", distance);
//									PWMUpdateMotorRight(RightDutyCycle);
//							    PWMUpdateMotorLeft(LeftDutyCycle);
//									printf("\r\n backward");
//                  ReturnEvent.EventType = ES_NO_EVENT;
//                  lastevent = backward;
//							 		LeftTargetRPM = (int16_t)(LeftDutyCycle*RPM_DC_RATIO);
//						      RightTargetRPM = (int16_t)(RightDutyCycle*RPM_DC_RATIO);	
//                  break;


						 case CLOCKWISE : 
									InitPIControl();							 
						 			LeftDutyCycle = -30;
									RightDutyCycle = 30;
									if(CurrentEvent.EventParam < 1){
										AngleToTurn = 20.0;
									}else{
										AngleToTurn = CurrentEvent.EventParam*1.0;
									}	
									PWMUpdateMotorRight(RightDutyCycle);
							    PWMUpdateMotorLeft(LeftDutyCycle);
									printf("\r\n rotate clockwise %i", CurrentEvent.EventParam);
									printf("\r\n rotate clockwise %f", AngleToTurn);
                  ReturnEvent.EventType = ES_NO_EVENT;
                  lastevent = clockwise;
						      LeftTargetRPM = (int16_t)(LeftDutyCycle*RPM_DC_RATIO);
						      RightTargetRPM = (int16_t)(RightDutyCycle*RPM_DC_RATIO);							 
                  break;

						case COUNTERCLOCKWISE : 
									InitPIControl();
									LeftDutyCycle = 40;
									RightDutyCycle = -40; //-40
									if(CurrentEvent.EventParam < 1){
										AngleToTurn = 20.0;
									}else{
										AngleToTurn = CurrentEvent.EventParam*1.0;
									}							
									PWMUpdateMotorRight(RightDutyCycle);
							    PWMUpdateMotorLeft(LeftDutyCycle);
//						      printf("\r\n rotate counterclockwise %i", CurrentEvent.EventParam);
//									printf("\r\n rotate counterclockwise %f", AngleToTurn);
                  ReturnEvent.EventType = ES_NO_EVENT;
                  lastevent = counterclockwise;
						      LeftTargetRPM = (int16_t)(LeftDutyCycle*RPM_DC_RATIO);
						      RightTargetRPM = (int16_t)(RightDutyCycle*RPM_DC_RATIO);	
                  break;
						
						case ALIGNBEACON :
							//InitPIControl();
									LeftDutyCycle = -30;
									RightDutyCycle = 30;
									PWMUpdateMotorRight(RightDutyCycle);
							    PWMUpdateMotorLeft(LeftDutyCycle);
									lastevent = alignbeacon;
									//printf("\r\n rotate the motor to align the beacon");
                  ReturnEvent.EventType = ES_NO_EVENT;
									LeftTargetRPM = (int16_t)(LeftDutyCycle*RPM_DC_RATIO);
						      RightTargetRPM = (int16_t)(RightDutyCycle*RPM_DC_RATIO);
									break;

						case PWM_UPDATE:
							    SpeedControlState = GetSpeedControlState();
									if(SpeedControlState.LeftDirection){
										LeftDutyCycle = SpeedControlState.RequestedDutyL;
									}else{
										LeftDutyCycle = -SpeedControlState.RequestedDutyL;
									}
									if(SpeedControlState.RightDirection){
										RightDutyCycle = SpeedControlState.RequestedDutyR;
									}else{
										RightDutyCycle = -SpeedControlState.RequestedDutyR;
									}
									if(SpeedControlState.RequestedDutyL > 90 || SpeedControlState.RequestedDutyR > 90){
										ES_Event Event2Post;
										Event2Post.EventType = STOP;
										PostMasterSM(Event2Post);
										Event2Post.EventType = BUMPER;
										PostMasterSM(Event2Post);	
//										printf("\r\n hit the bumper");
									}else{
									//printf("\r\n new dutycycle right %i, left %i", RightDutyCycle, LeftDutyCycle);
									//printf("\r\n");
								 	PWMUpdateMotorRight(RightDutyCycle);
							    PWMUpdateMotorLeft(LeftDutyCycle);
									}
									ReturnEvent.EventType = ES_NO_EVENT;
									break;
						case 	DECELERATE:
									if(lastevent == forward){
										LeftTargetRPM = CurrentEvent.EventParam;
										RightTargetRPM = CurrentEvent.EventParam;
										ReturnEvent.EventType = ES_NO_EVENT;
									}
									break;
								case STOP :
									//printf("\r\n Stop the motor");
										//ES_Event Event2Post;
										//Event2Post.EventType = MOTION_COMPLETED;
										//PostMasterSM(Event2Post);
									Stop();
								//do not consume the event
								ReturnEvent.EventType = ES_NO_EVENT;
									break;	
								case ES_EXIT :
									Stop();
								//do not consume the event
								ReturnEvent.EventType = ES_NO_EVENT;
								break;
									
							}
								
									
							
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartMotorService ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
	InitPWM();
	LeftDutyCycle = 0;
	RightDutyCycle = 0;
	RightTargetRPM = 0;
	LeftTargetRPM = 0;
	lastAngle = 0;
	lastevent = no_event;
   // call the entry function (if any) for the ENTRY_STATE
   RunMotorService(CurrentEvent);
}


int16_t GetTargetRPMLeft(void){
	return LeftTargetRPM;
}
int16_t GetTargetRPMRight(void){
	return RightTargetRPM;
}

LastEvent_t GetLastEvent(void){
	return lastevent;
}
/***************************************************************************
 private functions
 ***************************************************************************/

static void InitPWM()
{
// Set PE3 & PF4 as digital outputs and set them low
	
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;     //Port E
while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_RCGCGPIO_R4) != SYSCTL_RCGCGPIO_R4);
HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (BIT3HI);
HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (BIT3HI);
HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT3HI);

	
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R5;     //Port F
while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_RCGCGPIO_R5) != SYSCTL_RCGCGPIO_R5);
HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT4HI);
HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= ( BIT4HI);
HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT4HI);	

	
volatile uint32_t Dummy;
	// start by enabling the clock to the PWM Module (PWM0)
HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
// enable the clock to Port E
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
// Select the PWM clock as System Clock/32
HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
// make sure that the PWM module clock has gotten going
while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
;
// disable the PWM while initializing
HWREG( PWM0_BASE+PWM_O_2_CTL ) = 0;
// program generator A to go to 0 at rising compare A, 1 on falling compare A
HWREG( PWM0_BASE+PWM_O_2_GENA) =
(PWM_2_GENA_ACTCMPAU_ZERO | PWM_2_GENA_ACTCMPAD_ONE );
// program generator B to go to 0 at rising compare B, 1 on falling compare B
HWREG( PWM0_BASE+PWM_O_2_GENB) =
(PWM_2_GENA_ACTCMPBU_ZERO | PWM_2_GENA_ACTCMPBD_ONE );
// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period
HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((PeriodInMS * PWMTicksPerMS)-1)>>1;

// Set the initial Duty cycle on A to 50% by programming the compare value
// to 1/2 the period to count up (or down)
//HWREG( PWM0_BASE+PWM_O_2_CMPA) = ((PeriodInMS * PWMTicksPerMS)-1)>>2;
HWREG( PWM0_BASE+PWM_O_2_CMPA) = HWREG( PWM0_BASE+PWM_O_2_LOAD);
// Set the initial Duty cycle on B to 25% by programming the compare value
// to 1/4 the period
// HWREG( PWM0_BASE+PWM_O_0_CMPB) = ((PeriodInMS * PWMTicksPerMS))>>3;
//HWREG( PWM0_BASE+PWM_O_2_CMPB) = ((PeriodInMS * PWMTicksPerMS)-1)>>2;
HWREG( PWM0_BASE+PWM_O_2_CMPB) = HWREG( PWM0_BASE+PWM_O_2_LOAD);
// enable the PWM outputs
HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN);
// Set the Output Enables to happen locally synchronized to counter=0
HWREG( PWM0_BASE+PWM_O_ENUPD) = (HWREG( PWM0_BASE+PWM_O_ENUPD) &
~(PWM_ENUPD_ENUPD0_M | PWM_ENUPD_ENUPD1_M)) |
(PWM_ENUPD_ENUPD0_LSYNC | PWM_ENUPD_ENUPD1_LSYNC);
// now configure the Port B pins to be PWM outputs
// start by selecting the alternate function for Pe4 & 5
HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= (BIT4HI|BIT5HI);
// now choose to map PWM to those pins, this is a mux value of 4 that we
// want to use for specifying the function on bits 4 & 5
HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) =
(HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) & 0xff00ffff) + (4<<(5*BitsPerNibble)) + (4<<(4*BitsPerNibble));
// Enable pins 4 & 5 on Port E for digital I/O
HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (BIT4HI | BIT5HI);
// make pins 4 & 5 on Port E into outputs
HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (BIT4HI |BIT5HI);

// set the up/down count mode and enable the PWM generator, both generator updates locally synchronized to zero count
HWREG(PWM0_BASE+ PWM_O_2_CTL) |= (PWM_2_CTL_MODE | PWM_2_CTL_ENABLE | PWM_2_CTL_GENAUPD_LS | PWM_2_CTL_GENBUPD_LS);
printf("\r\n init PWM");
}





static void PWMUpdateMotorLeft(int32_t DutyLeft)
{
	if (DutyLeft < 0) {
		// Set PE3 to high, use the lower part of PWM
	  HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT3HI;
		
		if(DutyLeft <= -100){
			HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ZERO | PWM_2_GENB_ACTCMPBD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPB) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) >> 1;
		}		
		else {
			HWREG(PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPB) = (HWREG(PWM0_BASE+PWM_O_2_LOAD))*(-DutyLeft)/100;
		}
		
	}
	else {
		// Set PE3 to low, use the higher part of PWM
	  HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT3HI;
		if (DutyLeft == 0){
			HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ZERO | PWM_2_GENB_ACTCMPBD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPB) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) >> 1;
		} 
		else if(DutyLeft >= 100){
			HWREG( PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ONE );
			HWREG(PWM0_BASE+PWM_O_2_CMPB) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) >> 1;
		} else {
		  HWREG(PWM0_BASE+PWM_O_2_GENB) = (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPB) = (HWREG(PWM0_BASE+PWM_O_2_LOAD))*(100-DutyLeft)/100;
		}

	}
}

static void PWMUpdateMotorRight(int32_t DutyRight)
{
	if (DutyRight < 0) {
		// Set PF4 to high, use the lower part of PWM
	  HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT4HI;
		
		if(DutyRight <= -100){
			HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ZERO | PWM_2_GENA_ACTCMPAD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPA) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) / 2;
		}		
		else {
			HWREG(PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPA) = (HWREG(PWM0_BASE+PWM_O_2_LOAD))*(-DutyRight)/100;
		}
		
	}
	else {
		// Set PF4 to low, use the higher part of PWM
	  HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT4HI;
		if (DutyRight == 0){
			HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ZERO | PWM_2_GENA_ACTCMPAD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPA) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) >> 1;
		} 
		else if(DutyRight >= 100){
			HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ONE );
			HWREG(PWM0_BASE+PWM_O_2_CMPA) = (HWREG(PWM0_BASE+PWM_O_2_LOAD)) >> 1;
		} else {
		  HWREG(PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO);
			HWREG(PWM0_BASE+PWM_O_2_CMPA) = (HWREG(PWM0_BASE+PWM_O_2_LOAD))*(100-DutyRight)/100;
		}

	}
}

void Stop(void){
	LeftDutyCycle = 0;
	RightDutyCycle = 0;
	PWMUpdateMotorRight(0);
	PWMUpdateMotorLeft(0);
	LeftTargetRPM = LeftDutyCycle;
	RightTargetRPM = RightDutyCycle;
	printf("\r\n Stop");
	lastAngle = AngleToTurn;
	distance = 0.0;
	AngleToTurn = 0.0;
}

float Getdistance (void){
	return distance;
}
float Getangle (void){
	return AngleToTurn;
}

