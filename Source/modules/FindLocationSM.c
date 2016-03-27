/****************************************************************************
 Module
    3_FindLocationSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

#include "FindLocationSM.h"

/* Next level*/
#include "AlignBeacon.h"
#include "MotorService.h"
#include "Lidar.h"
/*----------------------------- Module Defines ----------------------------*/


/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringAlign( ES_Event Event);
static ES_Event DuringGetLocation( ES_Event Event);
uint16_t QueryCurrentPosition(void);

/*---------------------------- Module Variables ---------------------------*/
//static FindLocationState_t CurrentState;
static uint8_t numBeaconAligned;
static uint8_t frequency[2];
static float distance[2];
static float Xcoord;
static float Ycoord;
static uint8_t currentLocation;
static FindLocationState_t CurrentState;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunFindLocationSM
****************************************************************************/
ES_Event RunFindLocationSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   FindLocationState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
         case Align :
           
         CurrentEvent = DuringAlign(CurrentEvent);
           
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {

                if(numBeaconAligned == 2){
                  NextState = GetLocation;
                  MakeTransition = true;
                  EntryEventKind.EventType = ES_ENTRY;
                  
                }else{
                  ES_Event Event2Post;
                  Event2Post.EventType = ALIGNBEACON;
				  PostMasterSM(Event2Post);
				}
            ReturnEvent.EventType = ES_NO_EVENT;
         }
         break;

        case GetLocation :
           
        CurrentEvent = DuringGetLocation(CurrentEvent);
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case FAIL_TO_GET_LOCATION :
                  NextState = Align;
                  MakeTransition = true;
                  EntryEventKind.EventType = ES_ENTRY;

                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
            }
         }
         break;
    }
    
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunFindLocationSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunFindLocationSM(EntryEventKind);
     }
     return(ReturnEvent);
}


/****************************************************************************
 Function
     StartFindLocationSM
****************************************************************************/
void StartFindLocationSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = Align;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunFindLocationSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryFindLocationSM
****************************************************************************/
FindLocationState_t QueryFindLocationSM ( void )
{
   return(CurrentState);
}

uint16_t QueryCurrentPosition(void){
	return currentLocation;
}
/***************************************************************************
 During functions
 ***************************************************************************/

static ES_Event DuringAlign( ES_Event Event)
{
    ES_Event ReturnEvent = Event;

    //ENTRY - ROTATE ************************
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        StartAlignBeaconSM(Event);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        //Do nothing
        
   // }else if (Event.EventType == LOCATION_GET)
        
  //  {
//      ES_Event Event2Post;
//      Event2Post.EventType = ALIGNBEACON;
//      PostMasterSM(Event2Post);
//      /*--------------------------------------------
//      Record the frequency and the distance
//
//      ----------------------------------------------*/
//      numBeaconAligned++;
//      ES_Event Event2Post;
//      Event2Post.EventType = BEACON_ALIGNED;
//			printf("\r\n Aligned %d beacon",numBeaconAligned);
//      PostMasterSM(Event2Post);
//
//    }else{
//      RunMotorService(Event);
//      ReturnEvent = RunAlignBeaconSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringGetLocation( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
      currentLocation = 1;
        // implement any entry actions required for this state machine
        /*--------------------------------------------------
        Calculate and store the current location


        ----------------------------------------------------*/
        ES_Event Event2Post;
        if(currentLocation != 0){
          Event2Post.EventType = GET_LOCATION;
          Event2Post.EventParam = currentLocation;
          PostMasterSM(Event2Post);
        }else{
          Event2Post.EventType = FAIL_TO_GET_LOCATION;
          PostMasterSM(Event2Post);
        }
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
