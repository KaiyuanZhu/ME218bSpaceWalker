/****************************************************************************
 Module
   MasterSM.c

 Revision
   2.0.1

 Description
   This is a template for the top level Hierarchical state machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MasterSM.h"
#include "GamePlaySM.h"
#include "TOPPAC.h"
#include "Mapkeys.h"

/*----------------------------- Module Defines ----------------------------*/
#define ONE_SEC  976
#define GAME_TIME 47*ONE_SEC   //138 sec/timer_divide
/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringGamePlay( ES_Event Event);
static ES_Event DuringWaitForStart(ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static MasterState_t CurrentState;
static uint8_t timer_divide;
/*------------------------------ Module Code ------------------------------*/

bool InitMasterSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine

  StartMasterSM( ThisEvent );

  return true;
}


bool PostMasterSM( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


ES_Event RunMasterSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   MasterState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
        case WaitForStart :       
				 
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
					 //printf("\r\n not no event");
		 
					 CurrentEvent = DuringWaitForStart(CurrentEvent);

            switch (CurrentEvent.EventType)
            {
							case GAME_STATUS_COLLECTED :
										if (GetGameStatus(11) == 1){
												NextState = GamePlay;
											  printf("\r\n begin the game, change state to game start");									
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY;
                        ReturnEvent.EventType = ES_NO_EVENT;
										}
										break;
							case ES_TIMEOUT:
								if(CurrentEvent.EventParam == GAME_TIMER){
											ES_Event Event2Post;
											Event2Post.EventType = GETTING_GAME_STATUS_TOPPAC;
											PostMasterSM(Event2Post);
											printf("\r\n Getting game status \r\n");
											ES_Timer_InitTimer(GAME_TIMER, 50);
											ReturnEvent.EventType = ES_NO_EVENT;
										}
										break;

            }
         }
         break;
				 
				case GamePlay :       
				 
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
					 //printf("\r\n not no event");
		 
					 CurrentEvent = DuringGamePlay(CurrentEvent);

            switch (CurrentEvent.EventType)
            {
										
							case ES_TIMEOUT:
								if(CurrentEvent.EventParam == GAME_TIMER){
									if(timer_divide < 4){
										timer_divide++;
										printf("\r\n time divide %i", timer_divide);
										ES_Timer_InitTimer(GAME_TIMER, GAME_TIME);
									}else{
												NextState = Idle2;
											  printf("\r\n change state to wait for start");									
                        MakeTransition = true; 
                        EntryEventKind.EventType = ES_ENTRY;
                        ReturnEvent.EventType = ES_NO_EVENT;
												ES_Event Event2Post;
										Event2Post.EventType = STOP;
										PostMasterSM(Event2Post);
										}
									}
										break;

            }
         }
         break;
				 
				 				case Idle2 :       
	
         break;
				 
	
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunMasterSM(CurrentEvent);

       CurrentState = NextState; 

       RunMasterSM(EntryEventKind);
     }
   return(ReturnEvent);
}

void StartMasterSM ( ES_Event CurrentEvent )
{

   volatile ES_Event LocalEvent = CurrentEvent;
	StartTopPAC(CurrentEvent);
	CurrentState = WaitForStart;
	timer_divide = 0;
	printf("\r\n start run master");
   RunMasterSM(LocalEvent);
   return;
}


/***************************************************************************
 private functions
 ***************************************************************************/
static ES_Event DuringWaitForStart( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {								   

        	 ES_Event Event2Post;
					 Event2Post.EventType = GETTING_GAME_STATUS_TOPPAC;
					RunTopPAC(Event2Post);
			    printf("\r\n Getting game status \r\n");
					ES_Timer_InitTimer(GAME_TIMER, 50);
    
    }else if ( Event.EventType == ES_EXIT )
    {
       RunTopPAC(Event); 

			

    }else
    // do the 'during' function for this state
    {			

       ReturnEvent = RunTopPAC(Event); 
    }
    return(ReturnEvent);
}

static ES_Event DuringGamePlay( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			StartGamePlaySM(Event);
			ES_Timer_InitTimer(GAME_TIMER, GAME_TIME);
			timer_divide = 1;
    }
    else if ( Event.EventType == ES_EXIT )
    {
     RunTopPAC(Event);     
			RunGamePlaySM(Event);
      
    }else
    {
      ReturnEvent = RunTopPAC(Event);

			ReturnEvent = RunGamePlaySM(Event);

    }

    return(ReturnEvent);
}
