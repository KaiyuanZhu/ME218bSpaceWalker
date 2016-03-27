/****************************************************************************
Module
ShootingSM.c

Revision
2.0.1

Description
This is a template file for implementing state machines.

Notes

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
#include "ShootingSM.h"
#include "MotorService.h"
#include "AlignBeacon.h"
#include "GamePlaySM.h"
#include "PWMServo.h"
#include "PWMShooter.h"
#include "GoToShootingStationSM.h"
#include "RPMControl.h"
#include "TopPAC.h"
#include "MasterSM.h"
#include "AlignBeacon.h"


/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Wait2Shoot
#define ALL_BITS (0xff<<2)
#define ONE_SEC  976
#define SHOOT_TIME 4*ONE_SEC
#define QUERY_TIME ONE_SEC
#define SERVO_TIME ONE_SEC
#define BLUE 1
#define RED 0
#define PeriodInMS 1
#define BitsPerNibble 4
#define ALL_BITS (0xff<<2)  
#define AlignPin GPIO_PIN_0
#define PI 3.14159265
#define IRTHRE
#define NW 2
#define NE 1
#define SE 4
#define SW 3
#define SEPeriod 32000
#define NEPeriod 23529
#define NWPeriod 27586
#define SWPeriod 20513
#define CountNumNE 5
#define CountNumNW 5
#define CountNumSE 5
#define CountNumSW 5
#define toler 300
#define tolerSE 400

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
functions, entry & exit functions.They should be functions relevant to the
behavior of this state machine
*/
static ES_Event DuringFindCurrentLocationBucket(ES_Event Event);
static ES_Event DuringDriveToShootingPosition( ES_Event Event);
static ES_Event DuringReadyToShoot( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static ShootingState_t CurrentState;
static uint8_t shoot_trial;
static uint8_t shoot_color;
static uint8_t numBeaconAligned;
static uint32_t ThisCapture;
static uint32_t Period;
static uint8_t NECount;
static uint8_t SECount;
static uint8_t SWCount;
static uint8_t NWCount;
//static float map[2][2] = {{61.5,181.5},{181.5,61.5}};//RED ATTACK BUCKET, BLUE ATTACK BUCKET
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************

****************************************************************************/
ES_Event RunShootingSM( ES_Event CurrentEvent )
{
    bool MakeTransition = false;/* are we making a state transition? */
    ShootingState_t NextState = CurrentState;
    ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
		ES_Event Event2Post;
    
    switch ( CurrentState )
    {
			  case Wait2Shoot:
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            if(CurrentEvent.EventType == START_SHOOT){              
                NextState = FindCurrentShootingLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
            }
        }				  
			  break;
				
        case FindCurrentShootingLocation : 
        CurrentEvent = DuringFindCurrentLocationBucket(CurrentEvent);
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            if(CurrentEvent.EventType == OBTAIN_LOCATION){
                printf("\r\n get obtain location");
                NextState = DriveToShootingPosition;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
            }else if(CurrentEvent.EventType == LOCATION_NOT_FOUND){
             printf("\r\n location not found");
                //CurrentState = GoToPollingStation;
							ES_Event Event2Post;
						 uint16_t rotateAngle  =CurrentEvent.EventParam;
						   Event2Post.EventType = COUNTERCLOCKWISE;
							Event2Post.EventParam = rotateAngle;
							 PostMasterSM(Event2Post);
						 printf("\r\n rotate %i", rotateAngle);
						 ReturnEvent.EventType = ES_NO_EVENT;
								//PostMasterSM(EntryEventKind);						 
					 }else if(CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == counterclockwise){
						 							ES_Event Event2Post;
						   Event2Post.EventType = FORWARD;
							Event2Post.EventParam = 40;
						 printf("\r\n forward 20");
							 PostMasterSM(Event2Post);
							ReturnEvent.EventType = ES_NO_EVENT;
					 }else if(CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == forward){
						 printf("\r\n go back to find current shooting location");
								NextState = FindCurrentShootingLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
				 }
        }
        break;
        
        case DriveToShootingPosition:       // If current state is state one
        
        CurrentEvent = DuringDriveToShootingPosition(CurrentEvent);
        //process any events
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            switch (CurrentEvent.EventType)
            {
                case ARRIVE_LOCATION :
                // Execute action function for state one : event one
                NextState = ReadyToShoot;//Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; //mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
								printf("\r\n Arrive Location at shooting");
                break;
								
							case ARRIVE_FAIL:
								NextState = FindCurrentShootingLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
							printf("\r\n find current location again");
					 break;
					 
			}
		}
        break;
        
        case ReadyToShoot :       
				//printf("\n\r In ReadytoShoot");
        CurrentEvent = DuringReadyToShoot(CurrentEvent);
				//printf("\n\r In ReadytoShoot after During");
				
        //process any events
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT: 
								 if (CurrentEvent.EventParam == SHOOT_TIMER) 
							  {                 
                  NextState = ReadyToShoot;
                  MakeTransition = true;  //Still stay in here, don't make transition 
                  //SetServoPosition (0);  // Set Servo Position, release one ball  
									//SetServoPosition (0);
									//SetServoPosition (0); // give it some time to release the ball
                  SetServoPosition (60);	// Load Ball					
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
									ES_Timer_InitTimer(SERVO_TIMER, SERVO_TIME);
								  //
							   }
							
							   if (CurrentEvent.EventParam == QUERY_TIMER)
								 {
									Event2Post.EventType = GETTING_GAME_STATUS_TOPPAC;
								  PostMasterSM(Event2Post);
									ReturnEvent.EventType = ES_NO_EVENT;
									MakeTransition = true;
									NextState = BallShot;
									SetShooterSpeed(0); 
									//SetServoPosition (0);
								 }
								 
								 if (CurrentEvent.EventParam == SERVO_TIMER)
								 {
									ReturnEvent.EventType = ES_NO_EVENT;
									MakeTransition = true;
									NextState = ReadyToShoot;
									SetShooterSpeed(0); 
									SetServoPosition (0);
									ES_Timer_InitTimer(QUERY_TIMER, QUERY_TIME); // let the ball fly
								 }

								 
							break;
							 case BUCKET_ALIGNED:
                  ReturnEvent.EventType = ES_NO_EVENT;
							    ES_Timer_InitTimer(SHOOT_TIMER, SHOOT_TIME);// Let the motor speed up
							    SetShooterSpeed(100); // set the speed of the shooter							 			
           break;
							 
							 case BEACON_ALIGNED:
				 if(CurrentEvent.EventParam == GetTargetBucket()){
					 printf("TargetBucket code is %d",GetTargetBucket() );
					 HWREG(WTIMER0_BASE+TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
					 ES_Event Event2Post;
					 Event2Post.EventType = BUCKET_ALIGNED;
					 PostMasterSM(Event2Post);
					 Event2Post.EventType = STOP;
				   PostMasterSM(Event2Post);
					 ReturnEvent.EventType = ES_NO_EVENT;
					 printf("\r\n BucketAligned event");
				 }
				 break;
			 }
        }
        break;
				
        case BallShot :   
       // CurrentEvent = DuringBallShot(CurrentEvent);
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            switch (CurrentEvent.EventType)
            {
               case GAME_STATUS_COLLECTED: 
                  if (shoot_color == BLUE){
										if (GetGameStatus(9) == 1){ // Attack Succeeds
											 Event2Post.EventType = ATTACK_SUCCEED;
											 PostMasterSM(Event2Post);
										} else {
											 Event2Post.EventType = ATTACK_NOT_SUCCEED;
											 PostMasterSM(Event2Post);											
										}
									} else {
										if (GetGameStatus(10) == 1){ // Attack Succeeds
											 Event2Post.EventType = ATTACK_SUCCEED;
											 PostMasterSM(Event2Post);
										} else {
											 Event2Post.EventType = ATTACK_NOT_SUCCEED;
											 PostMasterSM(Event2Post);											
										}										
									}
									ReturnEvent.EventType = ES_NO_EVENT;
									MakeTransition = true;		
                  NextState = Wait2Shoot;									               									
							 break;
			
           }
        }
        break;				
				
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunShootingSM(CurrentEvent);
        
        CurrentState = NextState; //Modify state variable
        
        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunShootingSM(EntryEventKind);
    }
    return(ReturnEvent);
}
/****************************************************************************

****************************************************************************/
void StartShootingSM ( ES_Event CurrentEvent )
{

    if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
    {
        CurrentState = ENTRY_STATE;
    }
    uint8_t LEDColor = GetLEDColor();
    if (LEDColor == 0x00){//RED
        shoot_color = BLUE;
    } else {
        shoot_color = RED;
    }
    printf("\r\n start run shooting");
    // call the entry function (if any) for the ENTRY_STATE
    RunShootingSM(CurrentEvent);
}

/****************************************************************************

****************************************************************************/
ShootingState_t QueryShootingSM ( void )
{
    return(CurrentState);
}

/***************************************************************************
private functions
***************************************************************************/
static ES_Event DuringFindCurrentLocationBucket( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    if ( (Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY) )
    {
        StartAlignBeaconSM(Event);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        RunAlignBeaconSM(Event);
    }else{
        ReturnEvent = RunAlignBeaconSM(Event);
        ReturnEvent = RunMotorService(Event);    
    }
    return(ReturnEvent);
}

static ES_Event DuringDriveToShootingPosition( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY) )
    {
        StartGoToShootingStationSM(Event);
        StartRPMControl(Event);
        
        printf("\r\n StartGoToShootingStationSM");
    }
    else if ( Event.EventType == ES_EXIT )
    {
        RunGoToShootingStationSM(Event);
        RunRPMControl(Event);
        
    }else {
        ReturnEvent = RunGoToShootingStationSM(Event);
        ReturnEvent = RunRPMControl(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringReadyToShoot( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        //call shoot ball function
			Init_IRCapture();
					 ES_Event Event2Post;
		 Event2Post.EventType = ALIGNBEACON;
		 PostMasterSM(Event2Post);
			ES_Timer_InitTimer(SHOOT_FREEZE_TIMER, 3*ONE_SEC);
        printf("\r\n shoot ---");
			 // StartAlignBeaconSM(Event);
       
        shoot_trial = 0;
			  
			  //SetServoPosition (0); // do not load ball yet
    }
    else if ( Event.EventType == ES_EXIT )
    {      
			//	RunAlignBeaconSM(Event);
        shoot_trial = 0;
			  //SetShooterSpeed(0); 
			  //SetServoPosition (0);
		}
 

    else{
        
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
