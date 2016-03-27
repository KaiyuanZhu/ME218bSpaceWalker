/****************************************************************************
 Module
   GamePlaySM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "termio.h"
#include "ES_Port.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
 #include "BITDEFS.h"
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GamePlaySM.h"
#include "RPMControl.h"
#include "DrivingSM.h"
#include "ShootingSM.h"   
#include "TOPPAC.h"
#include "MotorService.h"
#include "MasterSM.h"
#include "PWMShooter.h"
#include "PWMServo.h"
/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines
#define ALL_BITS (0xff<<2)
#define ONE_SEC  976
#define FREEZE_TIME ONE_SEC*35 // set it very long first 
#define ENTRY_STATE Driving
#define LEDBLUE 0
#define LEDRED 1
#define Champaigning 1
#define Waiting 0
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringDriving( ES_Event Event);
static ES_Event DuringShooting( ES_Event Event);
static void InitLEDPins(void);
uint8_t GetLEDColor(void);
uint8_t GetFreezeTimerFlag(void);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GamePlayState_t CurrentState;
static uint8_t FreezeTimerFlag;
static uint8_t LEDColor;
static uint8_t TotalBallShoot;
/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     StartGamePlaySM
****************************************************************************/
void StartGamePlaySM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
       CurrentState = Driving;
		   //CurrentState = Driving;
   }
	 
	 InitLEDPins();
	 InitPWMShooter();
	 InitPWMServo();
	 SetServoPosition(0);
   FreezeTimerFlag = 0;
   // LEDColor = 0x00;// Red for BLue pass in 0x30
   TotalBallShoot = 0;
	 printf("\r\n start run game play");
   // call the entry function (if any) for the ENTRY_STATE
	 if (HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) == BIT5HI){ //RED LED
		 HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT3HI); 
		 LEDColor = 0x00;// Red 00000000
		 //printf("\n\rRED VOTE");
	 }else {
		 HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI); // BLUE LED
		 LEDColor = 0x30;// BLue 00110000
		 //printf("\n\rBLUE VOTE");
	 }
   RunGamePlaySM(CurrentEvent);
}

static void InitLEDPins(void)
{
		HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1; // Enables Port B
		while((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) // wait till peripheral reports clock ready
		;
		// Enable Red LED
		HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT2HI); // PB2
		HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT2HI); // PB2
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT2HI); // Set it to 0 first
		
	  // Enable Blue LED
		HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT3HI); // PB3
		HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT3HI); // PB3
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT3HI); // Set it to 0 first
	
	  // Enable LED Button
		HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT5HI); // PB5
		HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= ~(BIT5HI); // PB5 // set as input
		
		//printf("\r\n LED Pins Inited\r\n");
		
}

/****************************************************************************
 Function
		StartGamePlaySM
****************************************************************************/


ES_Event RunGamePlaySM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   GamePlayState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	 ES_Event Event2Post;

   switch ( CurrentState )
   {
      

          case Driving :       
	        CurrentEvent = DuringDriving(CurrentEvent);
					if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
              case ES_TIMEOUT :
                if(CurrentEvent.EventParam == SHOOT_FREEZE_TIMER  && TotalBallShoot <3){
                  FreezeTimerFlag = 1;
									printf("\r\n shoot freeze timer expired");
									ReturnEvent.EventType = ES_NO_EVENT;
                }
                  break;
							case CHANGE_SHOOT_STATE :
									MakeTransition = true; 
									NextState = Shooting;
                  ReturnEvent.EventType = ES_NO_EVENT;
									Event2Post.EventType = START_SHOOT;
									PostMasterSM(Event2Post);
									printf("\r\n change to shoot state");
								break;

            }
         }
         break;

         case Shooting :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringShooting(CurrentEvent);
				 //TotalBallShoot++;
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ATTACK_SUCCEED: //If event is event one
                  printf("\r\n change state to driving");
                  TotalBallShoot=5;
                  NextState = Driving;
                  MakeTransition = true; 
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
							   	FreezeTimerFlag = 0;
                  ES_Timer_InitTimer(SHOOT_FREEZE_TIMER, FREEZE_TIME);	
									printf("\r\n set freeze timer");							 
                  break;
                
                case ATTACK_NOT_SUCCEED:
                  printf("\r\n change state to driving");
                  TotalBallShoot++;
                  NextState = Driving;
                  MakeTransition = true; 
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
								  FreezeTimerFlag = 0;
                  ES_Timer_InitTimer(SHOOT_FREEZE_TIMER, FREEZE_TIME);
								printf("\r\n set freeze timer");
                 break;
								
								case ES_TIMEOUT:
									if(CurrentEvent.EventParam == FREEZE_TIMER)
								 {
									 NextState = Driving;
                  MakeTransition = true; 
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
									FreezeTimerFlag = 0;
                  ES_Timer_InitTimer(SHOOT_FREEZE_TIMER, FREEZE_TIME);	
									printf("\r\n set freeze timer");	
								 }
            }
						
         }
         break;
      // repeat state pattern as required for other states
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunGamePlaySM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunGamePlaySM(EntryEventKind);
     }
     return(ReturnEvent);
}


GamePlayState_t QueryGamePlaySM ( void )
{
   return(CurrentState);
}

uint8_t GetLEDColor(void)
{
	return(LEDColor);
}

uint8_t GetFreezeTimerFlag(void){
	return FreezeTimerFlag;
}
/***************************************************************************
 private functions
 ***************************************************************************/



static ES_Event DuringDriving( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        StartDrivingSM(Event);
        StartMotorService(Event);
			 ES_Timer_InitTimer(SHOOT_FREEZE_TIMER, 35*ONE_SEC);
			FreezeTimerFlag = 0;
			SetShooterSpeed(0); 
			printf("\r\n set freeze timer");
        /*----------------------------------
        Call spining the fly wheel function

        ---------------------------------------*/

    }
    else if ( Event.EventType == ES_EXIT )
    {
       RunDrivingSM(Event);
       RunMotorService(Event);
				//RunRPMControl(Event);

    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      ReturnEvent = RunDrivingSM(Event);
			ReturnEvent = RunMotorService(Event);
			//ReturnEvent = RunRPMControl(Event);
        // repeat for any concurrent lower level machines
    }
    return(ReturnEvent);
}

static ES_Event DuringShooting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        StartShootingSM(Event);
        StartMotorService(Event);
			  //StartRPMControl(Event);
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
      RunMotorService(Event);
      RunShootingSM(Event);


    }else
    // do the 'during' function for this state
    {
      ReturnEvent = RunShootingSM(Event);
			ReturnEvent = RunMotorService(Event);
    }

    return(ReturnEvent);
}
