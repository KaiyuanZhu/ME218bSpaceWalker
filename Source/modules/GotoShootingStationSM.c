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
#include "GotoShootingStationSM.h"
#include "MotorService.h"
#include "AlignBeacon.h"
#include "TOPPAC.h"
#include "GamePlaySM.h"
#define ToCm 2.54
/*----------------------------- Module Defines ----------------------------*/
//#define ENTRY_STATE GetClosestStation
#define PI 3.14159265
#define NW 2
#define NE 1
#define SE 4
#define SW 3

/*---------------------------- Module Functions ---------------------------*/


//float GetBucketDistance (void);
//float GetBucketAngle (void);
uint8_t GetTargetBucket(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GoToShootingStationState_t CurrentState;
//static uint32_t MsPerDegree;

//static float distance;
static float Distance;
static float Xcoord;
static float Ycoord;
static float Angle;
static float Xdiff;
static float Ydiff;
//static float Anglediff;
static float AngleToTurn;
//static bool clock;
static uint8_t MyColor;
static uint8_t BucketCode;
static uint8_t TargetBucket;
static uint8_t bumperflag;

static float map[2][2] = {{93.007,160.833},{150.426,90.4144}};//NW BUCKET, SE BUCKET

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
Function
StartGoToPollingStationSM
****************************************************************************/
void StartGoToShootingStationSM ( ES_Event CurrentEvent )
{
    if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
    {
        CurrentState = GetToBucket;
        Xcoord = QueryX();
        Ycoord = QueryY();
        Angle = QueryAngle();
        printf("\r\n Get from AlignBeacon X %f, Y %f, Angle %f", Xcoord, Ycoord, Angle);
        MyColor = GetLEDColor();
			TargetBucket = 0;
			bumperflag = 0;
    }
    //printf("\r\n start go to polling station");
    CurrentEvent.EventType = ES_ENTRY;
    RunGoToShootingStationSM(CurrentEvent);
}

/****************************************************************************
Function
RunGoToShootingStationSM
****************************************************************************/
ES_Event RunGoToShootingStationSM( ES_Event CurrentEvent )
{
    bool MakeTransition = false;/* are we making a state transition? */
    GoToShootingStationState_t NextState = CurrentState;
    ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
    
    //GetToTarget - DrivingToStation - Stop*********************************************
    
    
    switch (CurrentState)
    {
        case GetToBucket : // Go to a specific Bucket
        
        if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
        {
            if (CurrentEvent.EventType == ES_ENTRY)
            {
                if (MyColor == 0x00){
										TargetBucket = SE;
                    Distance = (float)sqrt((map[1][0]-Xcoord)*(map[1][0]-Xcoord) + (map[1][1]-Ycoord)*(map[1][1]-Ycoord));
                    if(Distance < 0){
                        Distance = -Distance;
                    }
                    BucketCode = 1;
                } else {
									TargetBucket = NW;
                    Distance = (float)sqrt((map[0][0]-Xcoord)*(map[0][0]-Xcoord) + (map[0][1]-Ycoord)*(map[0][1]-Ycoord));
                    if(Distance < 0){
                        Distance = -Distance;
                    }
                    BucketCode = 0;
                }
                printf("\r\n distance to the Shooting Station is %f",Distance);
                
                
                Xdiff = map[BucketCode][0]-Xcoord;
                Ydiff = map[BucketCode][1]-Ycoord;
                
                float LocationAngle;
                
                if (Xdiff >= 0 && Ydiff >= 0) {
                    LocationAngle = -(atan(Xdiff/Ydiff))*180.0/PI;
                }else if (Xdiff >= 0 && Ydiff < 0) {
                    LocationAngle = (atan(Xdiff/(-Ydiff)))*180.0/PI+180.0;
                }else if (Xdiff < 0 && Ydiff < 0) {
                    LocationAngle = (atan(-Ydiff/(-Xdiff)))*180.0/PI + 90.0;
                }else if (Xdiff < 0 && Ydiff >= 0) {
                    LocationAngle = (atan(-Xdiff/Ydiff))*180.0/PI;
                }
                printf("\r\n Location Angle %f", LocationAngle);
						AngleToTurn = LocationAngle - Angle;
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
							printf("\r\n align to bucket %f", AngleToTurn);
						}
                NextState = DrivingToBucket;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT; // Note: Check if this consumes all ES_ENTRY
                // break;
            }
					}        
            break;
            
            case DrivingToBucket :
            //CurrentEvent = DuringDrivingToStation(CurrentEvent);
            if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
            {
                if (CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == counterclockwise)
                {
									if(bumperflag){
						 						ES_Event Event2Post;
												Event2Post.EventType = FORWARD;
									     Event2Post.EventParam = 40;
						           printf("\r\n forward 40");
							         PostMasterSM(Event2Post);
						         	ReturnEvent.EventType = ES_NO_EVENT;
									}else{
                    ES_Event Event2Post;
                    Event2Post.EventType = FORWARD;
										Event2Post.EventParam = (uint16_t)Distance;
                    PostMasterSM(Event2Post);	
										ReturnEvent.EventType = ES_NO_EVENT;
									printf("\r\n drive to bucket %f", Distance);
									}                    
                }else if (CurrentEvent.EventType == MOTION_COMPLETED && GetLastEvent() == forward){
									if(!bumperflag){
									  ES_Event Event2Post;
                    Event2Post.EventType = ARRIVE_LOCATION;
									  PostMasterSM(Event2Post);	
                    ReturnEvent.EventType = ES_NO_EVENT;
									printf("\r\n arrive shoot location");
									}else{
										ES_Event Event2Post;
                    Event2Post.EventType = ARRIVE_FAIL;
									  PostMasterSM(Event2Post);	
                    ReturnEvent.EventType = ES_NO_EVENT;
										bumperflag = 0;
									printf("\r\n arrive fail");
									}
									
								}else if(CurrentEvent.EventType == BUMPER){
									ES_Event Event2Post;
						    Event2Post.EventType = COUNTERCLOCKWISE;
							  Event2Post.EventParam = 180;
							  PostMasterSM(Event2Post);
					 		  ReturnEvent.EventType = ES_NO_EVENT;
									bumperflag = 1;
						    printf("\r\n bumper rotate 180");
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
        RunGoToShootingStationSM(EntryEventKind);
    }
    return(ReturnEvent);
}


/****************************************************************************
Function
QueryGoToPollingStationSM
****************************************************************************/
GoToShootingStationState_t QueryGoToShootingStationSM ( void )
{
    return(CurrentState);
}

uint8_t GetTargetBucket(void){
	return TargetBucket;
}

