/****************************************************************************
 Module
    2_GoToPollingStationSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include <math.h>

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "MasterSM.h"
#include "GamePlaySM.h"
#include "GoToPollingStationSM.h"
#include "MotorService.h"
#include "RPMControl.h"
#include "AlignBeacon.h"
#include "HallSensor.h"
#include "TOPPAC.h"
#define BLUE 1
#define RED 10
#define STATION_UNCLAIMED 0
#define ToCm 2.54
#define NumCityChosen 5
#define ONE_SEC  976
/*----------------------------- Module Defines ----------------------------*/
//#define ENTRY_STATE GetClosestStation
#define PI 3.14159265

/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringGetClosestStation( ES_Event Event);
static ES_Event DuringDrivingToStation( ES_Event Event);
static ES_Event DuringGetToTarget( ES_Event Event);
static void RotateClock (void);
static void RotateCounterClock (void);
//float Getdistance (void);
//float Getangle (void);
uint8_t GetForwardFlag(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GoToPollingStationState_t CurrentState;
static uint32_t MsPerDegree;

static float distance;
//static float Distance[3];
static float Xcoord;
static float Ycoord;
static float Angle;
static float City;
static uint8_t Citycode;
static float Xdiff;
static float Ydiff;
static float CityAngle;
//static float Anglediff;
static float AngleToTurn;
//static bool clock;
static uint8_t availCity[9];
static uint8_t ForwardFlag;
static uint8_t Mycolor;
static uint8_t LastCapturedStation;

static float map[9][3] = {{0001,6.0,55.8},{0010,9,88.5},{0011,29.1,75.8},{1000,33.3,53.8},
    {1001,45.3,29.4},{1010,61.8,60.8},{1011,80.8,9.2},{1100,81.5,55.2},{1101,87.6,72.6}};

//static uint8_t cityLinkedList[NumCityChosen] = {2, 1, 0, 3, 4};

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
    StartGoToPollingStationSM
 ****************************************************************************/
void StartGoToPollingStationSM ( ES_Event CurrentEvent )
{
    if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
    {
        CurrentState = GetToTarget;
        Xcoord = QueryX();
        Ycoord = QueryY();
        Angle = QueryAngle();
//			Citycode = 10;
			CityAngle = 0;
//			Citycode = cityLinkedList[0];
			ForwardFlag = 1;
			for (uint8_t i=0; i<9; i++){
				availCity[i] = 0;			
			}
			
			uint8_t MycolorCode = GetLEDColor();
			if (MycolorCode == 0x00){
				Mycolor = 10;
				printf("\r\n My color is red");
			} else {
				Mycolor = 1;
				printf("\r\n My color is Blue");
			}
			CurrentEvent.EventType = ES_ENTRY;
    }
		//printf("\r\n start go to polling station");
    RunGoToPollingStationSM(CurrentEvent);
}

/****************************************************************************
 Function
    RunGoToPollingStationSM
****************************************************************************/
ES_Event RunGoToPollingStationSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   GoToPollingStationState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 //GetToTarget - DrivingToStation - Stop*********************************************

	 
   switch (CurrentState)
   {
       case GetToTarget : // Go to a specific city
           
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
					if(CurrentEvent.EventType == ES_ENTRY){
					ES_Event Event2Post;
					 Event2Post.EventType = GETTING_GAME_STATUS_TOPPAC; // check new game status
					 PostMasterSM(Event2Post);
						ReturnEvent.EventType = ES_NO_EVENT;
					}else if (CurrentEvent.EventType == GAME_STATUS_COLLECTED){
						printf("\r\n In GetToTarget X %f, Y %f, Angle %f", Xcoord, Ycoord, Angle);
						
							for (uint8_t i=0; i<9; i++){
								if(GetGameStatus(i)!=Mycolor){
								availCity[i] = 1;
								}				
									printf("\r\n %i", GetGameStatus(i));
							}
							float nearDistance = 9999.999;
							for (uint8_t i=0; i<9; i++){
								if(availCity[i]){
									distance = (float)sqrt((map[i][1]*ToCm-Xcoord)*(map[i][1]*ToCm-Xcoord) + (map[i][2]*ToCm-Ycoord)*(map[i][2]*ToCm-Ycoord));
									if(distance <= nearDistance){
										Citycode = i;
										nearDistance = distance;
									}
								}
							}
							if(nearDistance > 400.0){
								printf("\r\n cannot get nearest distance ");
								nearDistance = 30.0;
							}
							switch (Citycode) {
								case 0: printf("\r\n nearest city is Sacramento");break;
								case 1: printf("\r\n nearest city is Seattle");break;
								case 2: printf("\r\n nearest city is Billings");break;
								case 3: printf("\r\n nearest city is Denver");break;
								case 4: printf("\r\n nearest city is Dallas");break;
								case 5: printf("\r\n nearest city is Chicago");break;
								case 6: printf("\r\n nearest city is Miami");break;
								case 7: printf("\r\n nearest city is DC");break;
								case 8: printf("\r\n nearest city is Concord");break;
								default: printf("no city is available");break;
								
							}
						
						
							distance = (float)sqrt((map[Citycode][1]*ToCm-Xcoord)*(map[Citycode][1]*ToCm-Xcoord) + (map[Citycode][2]*ToCm-Ycoord)*(map[Citycode][2]*ToCm-Ycoord));
								
							if(distance < 0){
								distance = -distance;
							}
							printf("\r\n distance to the target is %f",distance);
            
            Xdiff = map[Citycode][1]*ToCm-Xcoord;
            Ydiff = map[Citycode][2]*ToCm-Ycoord;
							printf("\r\n cityX %f, cityY %f",  map[Citycode][1]*ToCm, map[Citycode][2]*ToCm);
							printf("\r\n Xdiff %f, Ydiff %f", Xdiff, Ydiff);

            if (Xdiff >= 0 && Ydiff >= 0) {
								CityAngle = -(atan(Xdiff/Ydiff))*180.0/PI;
            }else if (Xdiff >= 0 && Ydiff < 0) {
								CityAngle = (atan(Xdiff/(-Ydiff)))*180.0/PI+180.0;
            }else if (Xdiff < 0 && Ydiff < 0) {
								CityAngle = (atan(-Ydiff/(-Xdiff)))*180.0/PI + 90.0;
            }else if (Xdiff < 0 && Ydiff >= 0) {
								CityAngle = (atan(-Xdiff/Ydiff))*180.0/PI;
            }
						printf("\r\n City Angle %f", CityAngle);
						AngleToTurn = CityAngle - Angle;
						if(AngleToTurn < 0){
							AngleToTurn = 360.0 + AngleToTurn;
								//printf("\r\n AngleToTurn counterclockwise%f", AngleToTurn);
								printf("\r\n AngleToTurn counterclockwise%i", (uint16_t)AngleToTurn);
								ES_Event Event2Post;
								Event2Post.EventType = COUNTERCLOCKWISE;
								Event2Post.EventParam = (uint16_t)AngleToTurn;
								PostMasterSM(Event2Post);
						}else{
						    //printf("\r\n AngleToTurn counterclockwise%f", AngleToTurn);
								printf("\r\n AngleToTurn counterclockwise%i", (uint16_t)AngleToTurn);
						    ES_Event Event2Post;
						    Event2Post.EventType = COUNTERCLOCKWISE;
								Event2Post.EventParam = (uint16_t)AngleToTurn;
						    PostMasterSM(Event2Post);
						}
//          case GET_DIRECTION :
            NextState = DrivingToStation;
            MakeTransition = true;
            EntryEventKind.EventType = ES_ENTRY;						
            ReturnEvent.EventType = ES_NO_EVENT;
           // break;
					}
        }
        break;
				

					

       case DrivingToStation :
         //CurrentEvent = DuringDrivingToStation(CurrentEvent);
          if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
          {
						if (CurrentEvent.EventType == MOTION_COMPLETED)
						{
             ES_Event Event2Post;
						 Event2Post.EventType = FORWARD;
						Event2Post.EventParam = (uint16_t)distance;
						printf("\r\n forward distance");
						 PostMasterSM(Event2Post);
						 Event2Post.EventType = VOTE_MOVING;
						 PostMasterSM(Event2Post);							
						}
          }
          break;
           
				}
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
//       RunGoToPollingStationSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunGoToPollingStationSM(EntryEventKind);
     }
     return(ReturnEvent);
}

/****************************************************************************
 Function
     QueryGoToPollingStationSM
****************************************************************************/
GoToPollingStationState_t QueryGoToPollingStationSM ( void )
{
   return(CurrentState);
}


/***************************************************************************
 private functions
 ***************************************************************************/



static ES_Event DuringGetToTarget( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
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

uint8_t GetForwardFlag(void){
	return ForwardFlag;
}

