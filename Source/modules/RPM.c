/****************************************************************************
 Module
   RPM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
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
#include "MasterSM.h"
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "RPM.h"
#include "MotorService.h"
//#include "GoToPollingStationSM.h"
#include "AlignBeacon.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define aveNum 10
#define ALL_BITS (0xff<<2)
#define PWMTicksPerMS 40000
#define PeriodInMS 1
#define ONE_SEC  976
#define BitsPerNibble 4
#define iGain 0.1
#define pGain 2.1
#define PI 3.141592
#define REV 0
#define FOR 1
#define D 3.94
#define RPM_DC_RATIO 0.2

/*---------------------------- Module Functions ---------------------------*/
void InitEncoderCapture( void );
void EncoderLeftResponse( void );
void EncoderRightResponse( void );
static void PIControl(void);
ControlState GetSpeedControlState(void);
//static float QueryAngle(void);
static float Querydistance(void);
/*---------------------------- Module Variables ---------------------------*/
static uint32_t periodR[aveNum];
static uint32_t avePeriodR;
static uint8_t counterR;
static uint32_t LastCaptureR;
static uint32_t rpmR;
static uint32_t absCounterR;
static float distSofarR;
static float lastDistSofarL;
//define variables for the left motor
static uint32_t periodL[aveNum];
static uint32_t avePeriodL;
static uint8_t counterL;
static uint32_t LastCaptureL;
static uint32_t rpmL;
static uint32_t absCounterL;
static float distSofarL;
static float lastDistSofarR;
static float IntegralTermL; /* integrator control effort */
static float IntegralTermR;
static float RPMErrorL; /* make static for speed */
static float RPMErrorR;
static float TargetRPMLeft;
static float TargetRPMRight;
static uint32_t lastPeriodL;
static uint32_t lastPeriodR;

static ControlState SpeedControlState;
static RPMState_t CurrentState;
/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     StartRPMSM
****************************************************************************/
void StartTemplateSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
		 		CurrentState = CONTROL;
				InitEncoderCapture();
				InitPIControl();   
	 }
   // call the entry function (if any) for the ENTRY_STATE
   RunRPMSM(CurrentEvent);
}


/****************************************************************************
 Function
		RunRPMSM
****************************************************************************/
ES_Event RunRPMSM( ES_Event CurrentEvent )
{
	  bool MakeTransition = false;/* are we making a state transition? */
    RPMState_t NextState = CurrentState;
    ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

		switch(CurrentState)
		{
			case CONTROL :
				
				if(CurrentEvent.EventType == ES_TIMEOUT){
						
           if(CurrentEvent.EventParam == ENCODER_TIMER){
							//printf("\r\nRPM are %i left %i right, Errors are %f left %f right",rpmL, rpmR, RPMErrorL, RPMErrorR);
						
							//distance control
							distSofarR = absCounterR/12.0/50*PI*3.937;
							distSofarL = absCounterL/15.7/50*PI*3.937;
							//printf("\r\n dist left %f dist right %f", distSofarL, distSofarR);
							float distance = Querydistance();
						
							if(TargetRPMLeft > 0 && TargetRPMRight > 0){ 
										float diff = distance - (distSofarL+distSofarR)/2.0;
										//printf("\r\n distance to destination %f", diff);
										if(diff < 0){
											ES_Event Event2Post;
											Event2Post.EventType = STOP;
											PostMasterSM(Event2Post);
											//Stop();
											ES_Timer_InitTimer(MOTOR_TIMER, ONE_SEC/10);						
											NextState = IDLE;
											MakeTransition = true;
											ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);
											printf("\r\n change to idle");
										}else if(diff < 0.5){
											ES_Event Event2Post;
											Event2Post.EventType = DECELERATE;
											uint16_t newRPMTarget = (uint16_t)(80.0*RPM_DC_RATIO/4);
											//printf("\r\n new rpm target %i", newRPMTarget);
											Event2Post.EventParam = newRPMTarget;												
											PostMasterSM(Event2Post);
											PIControl();
											ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);
										}else if(diff < 1.0){
											ES_Event Event2Post;
											Event2Post.EventType = DECELERATE;
											uint16_t newRPMTarget = (uint16_t)(80.0*RPM_DC_RATIO/2);
											//printf("\r\n new rpm target %i", newRPMTarget);
											Event2Post.EventParam = newRPMTarget;												
											PostMasterSM(Event2Post);
											PIControl();
											ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);
										}else{
											PIControl();
											ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);							
										}
					  		}else{
										float aveAngleSoFar = (distSofarR+distSofarL)/2.0/(D/2.0);
										float angle = PI; //QueryAngle();
										float ratio = aveAngleSoFar/angle;
										//printf("\r\n angle ratio %f", ratio);
										if (angle - aveAngleSoFar <= 0){
												//ES_Event Event2Post;
												//Event2Post.EventType = MOTION_COMPLETED;
												//PostMasterSM(Event2Post);
												Stop();
												ES_Timer_InitTimer(MOTOR_TIMER, ONE_SEC/100);
												NextState = IDLE; // NextState		
										}else{
												PIControl();
												ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);
										}
							  }
				   }
									
				   if(CurrentEvent.EventParam == INPUT_STOP_TIMER){
								ES_Timer_InitTimer(INPUT_STOP_TIMER, ONE_SEC/10);
								uint32_t newPeriodL;
								newPeriodL = avePeriodL;
								if(lastPeriodL == newPeriodL){
										rpmL=0;
										//printf("\r\nleft motor stops");
								}
								lastPeriodL = newPeriodL;			

								uint32_t newPeriodR;
								newPeriodR = avePeriodR;
						
								if(lastPeriodR == newPeriodR){
										rpmR=0;
										//printf("\r\nright motor stops");
								}
								lastPeriodR = newPeriodR;		
								ReturnEvent.EventType = ES_NO_EVENT;
					 }					
        //break;
			}
		break;
							 
		case IDLE:
							if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == MOTOR_TIMER){
									 NextState = CONTROL;
									 MakeTransition = true;
									 ES_Event Event2Post;
									 printf("\r\n change to control");
									 //ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/10);
									 Event2Post.EventType = MOTION_COMPLETED;
									 PostMasterSM(Event2Post);
									 ReturnEvent.EventType = ES_NO_EVENT;									 
							}	 
		break;
    }

//			NextState = STATE_TWO;//Decide what the next state will be
//      MakeTransition = true; //mark that we are taking a transition
//      EntryEventKind.EventType = ES_ENTRY_HISTORY;
//      ReturnEvent.EventType = ES_NO_EVENT;
//							 
		if (MakeTransition == true)
    {
       CurrentEvent.EventType = ES_EXIT;
       RunRPMSM(CurrentEvent);
       CurrentState = NextState; 
       RunRPMSM(EntryEventKind);
    }
    return(ReturnEvent);
}

///****************************************************************************
// Function
//     QueryTemplateSM
//****************************************************************************/
//TemplateState_t QueryTemplateSM ( void )
//{
//   return(CurrentState);
//}

/***************************************************************************
 public functions
 ***************************************************************************/
void InitPIControl(void){
	counterL = 0;
	avePeriodL=0;
	for(uint8_t i=0; i<aveNum; i++){
		periodL[i] = 0xffffffff;
	}
	
		counterR = 0;
	avePeriodR=0;
	for(uint8_t i=0; i<aveNum; i++){
		periodR[i] = 0xffffffff;
	}
	rpmL=0;
	rpmR=0;
 IntegralTermL=0.0;
	IntegralTermR=0.0;
	SpeedControlState.LeftDirection = FOR;
	SpeedControlState.RightDirection = FOR;
	SpeedControlState.RequestedDutyL = 0;
	SpeedControlState.RequestedDutyR = 0;
	absCounterR = 0;
	absCounterL = 0;
	lastDistSofarL = 0.0;
	lastDistSofarR = 0.0;
	lastPeriodL = 0;
	lastPeriodR = 0;
//	CurrentState =CONTROL;
		ES_Timer_InitTimer(ENCODER_TIMER, ONE_SEC/100);
	ES_Timer_InitTimer(INPUT_STOP_TIMER, ONE_SEC/10);
	printf("\r\n init pi control");
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringStateOne( ES_Event Event)
{
    ES_Event ReturnEvent = Event; 

    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			
    }
    else if ( Event.EventType == ES_EXIT )
    {

    }else
    {

    }
    return(ReturnEvent);
}

void InitEncoderCapture( void ){

// start by enabling the clock to the timer (Wide Timer 1)
HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
// enable the clock to Port C
HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
// since we added this Port C clock init, we can immediately start
// into configuring the timer, no need for further delay
// make sure that Timer A & Timer B are disabled before configuring
HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~(TIMER_CTL_TAEN | TIMER_CTL_TBEN);
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
HWREG(WTIMER1_BASE+TIMER_O_TAILR) = 0xffffffff;
HWREG(WTIMER1_BASE+TIMER_O_TBILR) = 0xffffffff;
// set up timer A & timer B in capture mode (TAMR=3, TAAMS = 0),
// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
HWREG(WTIMER1_BASE+TIMER_O_TAMR) = (HWREG(WTIMER1_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
HWREG(WTIMER1_BASE+TIMER_O_TBMR) = (HWREG(WTIMER1_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) | (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);
// To set the event to rising edge, we need to modify the TAEVENT, TBEVENT bits
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT, TBEVENT bits
HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;
// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port C bit 6 (WT1CCP0) and bit 7 (WT1CCP1)
HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT6HI|BIT7HI);
// Then, map bit 4's alternate function to WT1CCP0 & WT1CCP1
// 7 is the mux value to select WT1CCP0 & WT1CCP1, 24 to shift it over to the
// right nibble for bit 6 (4 bits/nibble * 6 bits) and 28 to shift it over to the right
//nible for bit 7
HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0x00ffffff) + (7<<24) + (7<<28);
// Enable pin on Port C for digital I/O
HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT6HI | BIT7HI);
// make pin 4 on Port C into an input
HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= (BIT6LO | BIT7LO);
// back to the timer to enable a local capture interrupt
HWREG(WTIMER1_BASE+TIMER_O_IMR) |= (TIMER_IMR_CAEIM | TIMER_IMR_CBEIM);
// enable the Timer A in Wide Timer 1 interrupt in the NVIC
// it is interrupt number 96 and 97 so appears in EN3 at bit 0 & 1
HWREG(NVIC_EN3) |= (BIT0HI | BIT1HI);
// make sure interrupts are enabled globally
__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL |TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
printf("\r\n finish init encoder input capture");

}



void EncoderRightResponse( void ){
uint32_t ThisCapture;
ES_Event PostEvent;
// start by clearing the source of the interrupt, the input capture event
HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
// now grab the captured value and calculate the period
ThisCapture = HWREG(WTIMER1_BASE+TIMER_O_TAR);
periodR[counterR] = ThisCapture - LastCaptureR;
counterR++;
absCounterR++;
if(counterR >= aveNum){
	counterR = 0;
					for(uint8_t i=0; i<aveNum; i++){
					avePeriodR += periodR[i];
				}
				avePeriodR = avePeriodR/aveNum;
				rpmR  = 1000000000/avePeriodR*60/12/25/50;
}
// update LastCapture to prepare for the next edge
LastCaptureR = ThisCapture;
}

void EncoderLeftResponse( void ){
uint32_t ThisCapture;
ES_Event PostEvent;
// start by clearing the source of the interrupt, the input capture event
HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CBECINT;
// now grab the captured value and calculate the period
ThisCapture = HWREG(WTIMER1_BASE+TIMER_O_TBR);
periodL[counterL] = ThisCapture - LastCaptureL;
counterL++;
absCounterL++;
if(counterL >= aveNum){
	counterL = 0;
					for(uint8_t i=0; i<aveNum; i++){
					avePeriodL += periodL[i];
				}
				avePeriodL = avePeriodL/aveNum;
				rpmL  = 1000000000/avePeriodL*60/15.7/25/50;
}
// update LastCapture to prepare for the next edge
LastCaptureL = ThisCapture;
}

static void PIControl(void){

// start by clearing the source of the interrupt
HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_TATOCINT;
HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
TargetRPMLeft = (float)GetTargetRPMLeft();
TargetRPMRight = (float)GetTargetRPMRight();

if(TargetRPMLeft >= 0){
	RPMErrorL = TargetRPMLeft - (float)rpmL;
	SpeedControlState.LeftDirection = FOR;
}else{
	RPMErrorL = -(float)rpmL-TargetRPMLeft;
	SpeedControlState.LeftDirection = REV;		
	}
IntegralTermL += iGain * RPMErrorL;
SpeedControlState.RequestedDutyL = (uint16_t)(pGain * ((RPMErrorL)+IntegralTermL));	
	//printf("\r\n request duty left %i", SpeedControlState.RequestedDutyL);
if(SpeedControlState.RequestedDutyL > 100 && RPMErrorL > 0){
	IntegralTermL -= iGain * RPMErrorL;
	SpeedControlState.RequestedDutyL=100;
}else if(SpeedControlState.RequestedDutyL < 0 && RPMErrorL < 0 ){
	IntegralTermL -= iGain * RPMErrorL;
	SpeedControlState.RequestedDutyL=0;
}



if(TargetRPMRight >= 0){
	RPMErrorR = TargetRPMRight - (float)rpmR;
	SpeedControlState.RightDirection = FOR;
}else{
	RPMErrorR = -(float)rpmR-TargetRPMRight;
	SpeedControlState.LeftDirection = REV;	
}
//printf("\r\n current rpm right %i left %i",rpmR, rpmL);
//printf("\r\n target rpm right %f left %f",TargetRPMRight,TargetRPMLeft);
//printf("\r\n error right %f left %f", RPMErrorR, RPMErrorL);
IntegralTermR += iGain * RPMErrorR;
SpeedControlState.RequestedDutyR = (uint16_t)(pGain * ((RPMErrorR)+IntegralTermR));
	//printf("\r\n request duty right %i", SpeedControlState.RequestedDutyR);
if(SpeedControlState.RequestedDutyR > 100 && RPMErrorR > 0){
	IntegralTermR -= iGain * RPMErrorR;
	SpeedControlState.RequestedDutyR=100;
}else if(SpeedControlState.RequestedDutyR < 0 && RPMErrorR < 0 ){
	IntegralTermR -= iGain * RPMErrorR;
	SpeedControlState.RequestedDutyR=0;
}
	ES_Event Event2Post;
	Event2Post.EventType = PWM_UPDATE;

	PostMasterSM(Event2Post);
}

ControlState GetSpeedControlState(void){
	return SpeedControlState;
}

//static float QueryAngle(void){
//return PI;
//}
static float Querydistance(void){ //????????????
	return 5.0;
}
