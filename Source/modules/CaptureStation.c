/****************************************************************************
 Module
   CaptureStation.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
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
#include "MotorService.h"
#include "GoToPollingStationSM.h"
#include "AlignBeacon.h"
#include "HallSensor.h"
#include "CaptureStation.h"
#include "TOPPAC.h"
/*----------------------------- Module Defines ----------------------------*/

#define ALL_BITS (0xff<<2)
#define ONE_SEC  976
#define SEARCH_TIME 5*ONE_SEC
#define CAPTURE_TIME 5*ONE_SEC

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringStateOne( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static CaptureStation_t CurrentState;
static uint8_t CaptureTimerFlag;
static uint8_t GameTimerFlag;
//static float Distance[3];
//static float Xcoord;
//static float Ycoord;
//static float Angle;
static uint8_t forwardflag;
static uint32_t ThisFreq;
/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     StartCaptureStationSM
****************************************************************************/
void StartCaptureStationSM ( ES_Event CurrentEvent )
{
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = SensorRead;
		 	  StartHallSensor(CurrentEvent);
		 CaptureTimerFlag = 0;
		 forwardflag = 1;
		 if(GetForwardFlag()){
					ES_Timer_InitTimer(FORWARD_TIMER, 0.2*ONE_SEC);
					forwardflag = 0;
					printf("\r\n set forwardflag to zero");
				}
		 		//ES_Event Event2Post;
			//	Event2Post.EventType = ALIGNBEACON;
			//	PostMasterSM(Event2Post);
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunCaptureStationSM(CurrentEvent);
}

/****************************************************************************
 Function
    RunCaptureStationSM
****************************************************************************/
ES_Event RunCaptureStationSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   CaptureStation_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	 
   switch ( CurrentState )
   {	 
		   case SensorRead :
				// printf("\r\n sensor read state");
			if(CurrentEvent.EventType != ES_NO_EVENT){
				 CurrentEvent = RunHallSensor(CurrentEvent);
				if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == FORWARD_TIMER){
					forwardflag = 1;
					printf("\r\n timeout, set forwardflag to one");
				}else if (forwardflag && CurrentEvent.EventType == HallSensorRead){
								ES_Event Event2Post;
						   Event2Post.EventType = STOP;
							 PostMasterSM(Event2Post);
							NextState = WaitForResponse1;
							ReturnEvent.EventType = ES_NO_EVENT;
							//uint8_t Freq = CurrentEvent.EventParam;
							printf("\r\n receive hall sensor read event, change state to wait");
							//ES_Event Event2Post;
							Event2Post.EventType = CHANGING_STATIONSTATUS_TOPPAC;
							Event2Post.EventParam = CurrentEvent.EventParam;
							PostMasterSM(Event2Post);
							ES_Timer_InitTimer(CAPTURE_TIMER, 100);
							printf("\r\n Changing station status request post to MasterSM");
						
						MakeTransition = true;
						}
						// Add a timer here, if the timer expire and no Hall Sensor Read, start to spin the car 360 degree, 
						//if still no capture, recalibrate and capture the nearest city, which should be the previous one
					}
			

			 break;
/*
			 case ReadSignal1 :
				 
						RunHallSensor(CurrentEvent);
			 
						if (CurrentEvent.EventType == HALLSENSORGET){							
									ES_Event Event2Post;
									Event2Post.EventType = CHANGING_STATIONSTATUS_TOPPAC;
									Event2Post.EventParam = CurrentEvent.EventParam;
									PostMasterSM(Event2Post);
							
									NextState = WaitForResponse1;
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
						}
						MakeTransition = true;
				break;
*/						
				case ReadSignal2 :
					//printf("\r\n readSignal2 state");
			if(CurrentEvent.EventType != ES_NO_EVENT){					
						 CurrentEvent = RunHallSensor(CurrentEvent); 

						 if (CurrentEvent.EventType == HallSensorRead){
								  ES_Event Event2Post;
									Event2Post.EventType = FREQUENCY_GOT;
									Event2Post.EventParam = CurrentEvent.EventParam;
									PostMasterSM(Event2Post);
               printf("\r\n post frequency_got event");
									NextState = WaitForResponse1;
                  EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;
						 
						 MakeTransition = true;
						 }
					 }
				break;
							 	
				case WaitForResponse1 :
					//printf("\r\n waitforresponse state");
				          if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == CAPTURE_TIMER){
											ES_Event Event2Post;
											Event2Post.EventType = CHANGING_STATIONSTATUS_TOPPAC;
											Event2Post.EventParam = CurrentEvent.EventParam;
											PostMasterSM(Event2Post);
						//	ES_Timer_InitTimer(CAPTURE_TIMER, 100);
							printf("\r\n Changing station status request post to MasterSM");
									}
                  else if (CurrentEvent.EventType == WAIT_FOR_FREQUENCY){
												MakeTransition = true;
										    //RunHallSensor(CurrentEvent);
												NextState = ReadSignal2;
										printf("\r\n wait for new freq, change state to readSignal2");	
							
									}else if (CurrentEvent.EventType == WRONG_FIRST_FREQUENCY){
//												ES_Event Event2Post;
//												Event2Post.EventType = CHANGING_STATIONSTATUS_TOPPAC;
//												PostMasterSM(Event2Post);
										printf("\r\n wrong first frequency");
												MakeTransition = true;										
												NextState = SensorRead;
										
									}else if (CurrentEvent.EventType == WRONG_SECOND_FREQUENCY){
										    //Re-Read the NEW Freq, NOT changing currentstate
												NextState = SensorRead;
												MakeTransition = true;
												printf("\r\n wrong second frequency");
									}else if (CurrentEvent.EventType == OCCUPY_BLOCKED){
												//Shoot?
												ES_Event Event2Post;
												Event2Post.EventType = GO_TO_SHOOTING;
												PostMasterSM(Event2Post);
												printf("\r\n occupy_blocked");
												NextState = SensorRead;
												MakeTransition = true;
									} 

									EntryEventKind.EventType = ES_ENTRY;
                  ReturnEvent.EventType = ES_NO_EVENT;					
				 break;
				}
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       CurrentEvent.EventType = ES_EXIT;
       RunCaptureStationSM(CurrentEvent);
       CurrentState = NextState; 
       RunCaptureStationSM(EntryEventKind);
     }
     return(ReturnEvent);
}



