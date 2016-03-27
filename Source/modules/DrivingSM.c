/****************************************************************************
 Module
   DrivingSM.c
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
#include "MasterSM.h"
#include "DrivingSM.h"
#include "GoToPollingStationSM.h"
#include "AlignBeacon.h"
#include "RPMControl.h"
#include "HallSensor.h"
#include "CaptureStation.h"
#include "MotorService.h"
#include "GamePlaySM.h"
/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

    //#define ENTRY_STATE FindCurrentLocation
#define ALL_BITS (0xff<<2)
#define ONE_SEC  976
#define SEARCH_TIME 5*ONE_SEC
#define CAPTURE_TIME 5*ONE_SEC
#define GOAWAY_TIME 6*ONE_SEC
/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringFindCurrentLocation( ES_Event Event);
static ES_Event DuringGoToPollingStation( ES_Event Event);
//static ES_Event DuringFindPollingStation( ES_Event Event);
static ES_Event DuringCapturePollingStation( ES_Event Event);
/*---------------------------- Module Variables ---------------------------*/
static DrivingState_t CurrentState;
//static float Distance[3];
static float Xcoord;
static float Ycoord;
static float Angle;
static uint8_t BackwardFlag = 0;
static uint32_t ThisFreq; //true if we catch original frequency of the City and wait for the new Freq from SuperPAC


/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
    StartDrivingSM
 ****************************************************************************/
void StartDrivingSM ( ES_Event CurrentEvent )
{

    if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
    {
        CurrentState = FindCurrentLocation;

    }
    printf("\r\n Start DrivingSM");

    RunDrivingSM(CurrentEvent);
}

/****************************************************************************
 Function
    RunDrivingSM
 ****************************************************************************/

ES_Event RunDrivingSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   DrivingState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case FindCurrentLocation :
         //printf("\r\n In DrivingSM - FindCurrentLocation state");

		  	 CurrentEvent = DuringFindCurrentLocation(CurrentEvent);
			if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
					 if(CurrentEvent.EventType == OBTAIN_LOCATION){
             printf("\r\n DrivingSM - FindCurrentLocation state finished, go to GoToPollingStation state");
                //CurrentState = GoToPollingStation;
								NextState = GoToPollingStation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
								//PostMasterSM(EntryEventKind);
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
							Event2Post.EventParam = 50;
						 printf("\r\n forward 50");
							 PostMasterSM(Event2Post);
							ReturnEvent.EventType = ES_NO_EVENT;
					 }else if(CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == forward){
								NextState = FindCurrentLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
				 }else if(CurrentEvent.EventType == BUMPER){
					 							ES_Event Event2Post;
						   Event2Post.EventType = COUNTERCLOCKWISE;
							Event2Post.EventParam = 180;
							 PostMasterSM(Event2Post);
					 		ReturnEvent.EventType = ES_NO_EVENT;
						 printf("\r\n rotate 180");
				 }
			 }
         break;
				 
           
       case GoToPollingStation :
					//printf("\r\n In DrivingSM - GoToPollingStation state");

         CurrentEvent = DuringGoToPollingStation(CurrentEvent);
			 
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
					 //printf("\r\n In DrivingSM - GoToPollingStation state");
					 if(CurrentEvent.EventType == VOTE_MOVING){
             printf("\r\n DrivingSM - GoToPollingStation state finished, go to CapturePollingStation state");
							
                NextState = CapturePollingStation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
					 }
         }
         break;

				 
			case CapturePollingStation :
        // printf("\r\n In DrivingSM - CapturePollingStation state");
			
				 CurrentEvent = DuringCapturePollingStation(CurrentEvent);
         
				 if(CurrentEvent.EventType == OCCUPY_IS_SUCCEED){
				 printf("\r\n In DrivingSM - City Capture Successful. Capture the next station");
					MakeTransition = true;
					NextState = FindCurrentLocation;
          EntryEventKind.EventType = ES_ENTRY;
					 EntryEventKind.EventParam = CurrentEvent.EventParam;
          ReturnEvent.EventType = ES_NO_EVENT;
				 
    }else if(CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == forward){
					if(!BackwardFlag){
							ES_Event Event2Post;
						   Event2Post.EventType = COUNTERCLOCKWISE;
							Event2Post.EventParam = 450;
							 PostMasterSM(Event2Post);
							ReturnEvent.EventType = ES_NO_EVENT;
					BackwardFlag = 1;
					printf("\r\n rotate 450 degree");
					}else{
						    NextState = FindCurrentLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
					BackwardFlag = 0;
						printf("\r\n recalibrate again");
					}
				}else if(CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == counterclockwise){
								ES_Event Event2Post;
								Event2Post.EventType = FORWARD;
								Event2Post.EventParam = 50;
								PostMasterSM(Event2Post);
								printf("\r\n forward 50cm");
				}else if(CurrentEvent.EventType == BUMPER){
					 							ES_Event Event2Post;
						   Event2Post.EventType = COUNTERCLOCKWISE;
							Event2Post.EventParam = 180;
							PostMasterSM(Event2Post);
					 		ReturnEvent.EventType = ES_NO_EVENT;
							printf("\r\n rotate 180");
							BackwardFlag = 1;
				 }else if((CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == GOAWAY_TIMER) || CurrentEvent.EventType == INVALID_RESPONSE_RECEIVED){
					  NextState = FindCurrentLocation;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
					 printf("\r\n go away timer expired, go back to find current location");
				 }
				 break;

	
	}
		
    if (MakeTransition == true)
    {
			//printf("\r\n make transition");
       CurrentEvent.EventType = ES_EXIT;
       RunDrivingSM(CurrentEvent);
       CurrentState = NextState;
       RunDrivingSM(EntryEventKind);
     }
    
	  return(ReturnEvent);
	 
}


/****************************************************************************
 Function
     QueryDrivingSM

****************************************************************************/
DrivingState_t QueryDrivingSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringFindCurrentLocation( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
      StartAlignBeaconSM(Event);
			StartRPMControl(Event);
    }
    else if ( Event.EventType == ES_EXIT )
    {
      RunAlignBeaconSM(Event);
			RunRPMControl(Event);
//								if(GetFreezeTimerFlag()){
//							ES_Event Event2Post;
//							Event2Post.EventType = CHANGE_SHOOT_STATE;
//							PostMasterSM(Event2Post);
//							printf("\r\n post change shoot state event");
//						}

    }else{
      ReturnEvent = RunAlignBeaconSM(Event);
			ReturnEvent = RunRPMControl(Event);
		  //ReturnEvent = RunMotorService(Event);

    }
    return(ReturnEvent);
}

static ES_Event DuringGoToPollingStation( ES_Event Event)
{
    ES_Event ReturnEvent = Event;

    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("\r\n call startGoToPollingStationSM");
        StartGoToPollingStationSM(Event);
				StartRPMControl(Event);
    }
    else if ( Event.EventType == ES_EXIT )
    {
      RunGoToPollingStationSM(Event);
			RunRPMControl(Event);

    }else
    {
      ReturnEvent = RunGoToPollingStationSM(Event);
			ReturnEvent = RunRPMControl(Event);
    }
    return(ReturnEvent);
}


static ES_Event DuringCapturePollingStation( ES_Event Event)
{
    ES_Event ReturnEvent = Event;
    if ( (Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY) )
    {
			  StartCaptureStationSM(Event);
			ES_Timer_InitTimer(GOAWAY_TIMER, GOAWAY_TIME);
			printf("\r\n set goaway timer");

    }
    else if ( Event.EventType == ES_EXIT )
    {
     		RunCaptureStationSM(Event);
				RunRPMControl(Event);
			if(GetFreezeTimerFlag()){
							ES_Event Event2Post;
							Event2Post.EventType = CHANGE_SHOOT_STATE;
							PostMasterSM(Event2Post);
							printf("\r\n post change shoot state event");
						}
   
    }else{ //if (Event.EventType == GET_RESPONSE)
				
				ReturnEvent = RunCaptureStationSM(Event);
				ReturnEvent = RunRPMControl(Event);
			//ReturnEvent = RunMotorService(Event);
        
    }
    return(ReturnEvent);
}


