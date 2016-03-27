
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BITDEFS.H"
#include "termio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ssi.h"
#include "SendByte.h"
#include "MasterSM.h"




/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "PACSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WAITING4COMMAND
#define BitsPerNibble 4
#define TicksPerMS    40000
#define ALL_BITS     (0xff<<2)
#define CPSDVSR 0x0014 //(20)
#define SCR 0x0009
#define NO_COMMAND	0x00
#define SENDCOMMAND_TIMER_LENGTH 1
#define BLUE 1
#define RED 10
#define GAME_WAITING 0
#define GAME_CHAMPAIGING 1
#define STATION_UNCLAIMED 0
#define RED_UNDERATTACK 1
#define BLUE_UNDERATTACK 1
#define NACK 0
#define ACK 1
#define BLOCKED 10
#define LOCATION 0000
#define BUSY 11
#define COLOR_UNDEFINED 11
#define COLOR_NOTCLAIMED 0
#define RESPONSE_NOT_READY 0
#define RESPONSE_READY 1
#define SACRAMENTO 1
#define SEATTLE 2
#define BILLINGS 3
#define DENVER 4
#define DALLAS 5
#define CHICAGO 6
#define MIAMI 7
#define WASHINGTONDC 8
#define CONCORD 9





/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting( ES_Event Event);
static ES_Event DuringSendCommand( ES_Event Event);
uint8_t GetQueryStatus(uint8_t); //0 - 2 
uint8_t GetResponseReady(uint8_t); //0
uint8_t GetWholeGameStatus(uint8_t); // 0 - 11
uint8_t GetFrequency(void); // for testing
ES_Event RunPACSM( ES_Event CurrentEvent);
void StartPACSM ( ES_Event CurrentEvent);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static PACState_t CurrentState;;
static uint8_t WholeGameStatus[12] = {0,0,0,0,0,0,0,0,0,0,0,0}; // (0)station 1, (1)station 2, (2)station 3, (3)station 4, station 5, station 6, station 7, station 8, station 9, (9)Attack AD Status RED, (10)Attack AD Status BLUE, (11)Game Status
static uint8_t ResponseReadByte[1] = {0}; // Ready 1, Not Ready 0
static uint8_t QueryStatusByte [3] = {0, 0, 0}; // Acknowledge, Color, Location
static uint8_t Freq = 0x0C;


ES_Event RunPACSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   PACState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	 ES_Event Event9Post;

   switch (CurrentState)
   {
       case WAITING4COMMAND :       // If current state is state one
			  CurrentEvent = DuringWaiting(CurrentEvent);
			 if ( CurrentEvent.EventType != ES_NO_EVENT ) 
			 {
           switch (CurrentEvent.EventType)
				{
           case OCCUPYCOMMAND_PACSM:
						  NextState = SENDOCCUPYCOMMAND;
					    MakeTransition = true; 
					    ReturnEvent.EventType = ES_NO_EVENT;
					    ES_Event Event2Post;
					    Event2Post.EventType = ES_SENDCOMMAND;
					    Event2Post.EventParam = CurrentEvent.EventParam;
					    PostMasterSM(Event2Post);
					     //printf("\n\r in OCCUPYCOMMAND_PACSM \n\r");	
					    
					 break;
					 
					 case STATUSCOMMAND_PACSM:
						  NextState = SENDCHECKSTATUS;
					    MakeTransition = true; 
					    ReturnEvent.EventType = ES_NO_EVENT;
 					    ES_Event Event3Post;
					    Event3Post.EventType = ES_SENDCOMMAND;
					    Event3Post.EventParam = CurrentEvent.EventParam;
					    PostMasterSM(Event3Post);     
              //printf("\n\r in STATUSCOMMAND_PACSM \n\r");					 
					 break;
					 
					 case QUERYCOMMAND_PACSM:
						  NextState =SENDQUERYCOMMAND;
					    MakeTransition = true; 
					    ReturnEvent.EventType = ES_NO_EVENT;		
					    ES_Event Event8Post;
					    Event8Post.EventType = ES_SENDCOMMAND;
					    Event8Post.EventParam = CurrentEvent.EventParam;
					    PostMasterSM(Event8Post);		
              //printf("\n\r in QUERYCOMMAND_PACSM \n\r");					 
					 break;
					 						
				 }						 
			 }
			 break;
			 
			case SENDOCCUPYCOMMAND:
						CurrentEvent = DuringSendCommand(CurrentEvent);
			      if (ES_TIMEOUT == CurrentEvent.EventType && BYTETIMEOUT_TIMER == CurrentEvent.EventParam )
						{
								NextState = WAITING4COMMAND;
					      MakeTransition = true; 
					      ReturnEvent.EventType = ES_NO_EVENT;
						   	ES_Event Event5Post;
                Event5Post.EventType = OCCUPY_SENT_PACSM; // has to post an event to get out of the case
								PostMasterSM(Event5Post);		
               // printf("\n\r in SENDOCCUPYCOMMAND \n\r");								
						}
            			
			break;
					 
			case SENDCHECKSTATUS:
						CurrentEvent = DuringSendCommand(CurrentEvent);
						if (ES_TIMEOUT == CurrentEvent.EventType && BYTETIMEOUT_TIMER == CurrentEvent.EventParam )
						{
							 	//printf("\n\r BYTETIMEOUT Occured, Gathering Status\n\r");	
								NextState = WAITING4COMMAND;
					      MakeTransition = true; 
					      ReturnEvent.EventType = ES_NO_EVENT;

							uint8_t Byte5Return = GetSPIByte(4);
//							uint8_t testbit = Byte5Return & BIT7HI;
//							uint8_t testbit2 = Byte5Return & BIT6HI;
//							
//							//printf("\n\r GetSPIByte(4) %d\n\r", testbit);	
//							//printf("\n\r GetSPIByte(4) %x\n\r", testbit2);	
		
							if ((Byte5Return & BIT0HI) == BIT0HI)
							{
								WholeGameStatus[11] = GAME_CHAMPAIGING;
								//printf("\n\r GAME_CHAMPAIGING; \n\r");	
							}
							else if ((Byte5Return & BIT0HI) == BIT0LO)
							{
								WholeGameStatus[11] = GAME_WAITING; 
								//printf("\n\r GAME_WAITING; \n\r");	
							}
							if ((Byte5Return & BIT2HI) == BIT2HI)
							{
								WholeGameStatus[10] = RED_UNDERATTACK;
								//printf("\n\r RED_UNDERATTACK; \n\r");	
							}
							
							if ((Byte5Return & BIT1HI) == BIT1HI)
							{
								WholeGameStatus[9] = BLUE_UNDERATTACK; 
								//printf("\n\r BLUE_UNDERATTACK; \n\r");	
							}
							
							if (((Byte5Return & BIT7HI) != BIT7HI) && ((Byte5Return & BIT6HI) == BIT6HI))
							{
								WholeGameStatus[8] = BLUE; // 0,1
								//printf("\n\r Station 9 Blue; \n\r");	
							}
							else if (((Byte5Return & BIT7HI) == BIT7HI) && ((Byte5Return & BIT6HI) != BIT6HI))
							{
								WholeGameStatus[8] = RED;// 1,0
								//printf("\n\r Station 9 red; \n\r");	
							}
							else if (((Byte5Return & BIT7HI) != BIT7HI) && ((Byte5Return & BIT6HI) != BIT6HI))
							{
								WholeGameStatus[8] = STATION_UNCLAIMED; // 0,0
								//printf("\n\r Station 9 STATION_UNCLAIMED; \n\r");
							} 
							else if (((Byte5Return & BIT7HI) == BIT7HI) && ((Byte5Return & BIT6HI) == BIT6HI))
							{
								//printf("\n\r Station 9 STATION UNDEFINED; \n\r");
							}
							else {
								//printf("\n\r Station 9 ELSE; \n\r");
								//printf("\n\r %x\n\r", Byte5Return);
							}
							
							for (uint8_t i = 1; i < 3; i++)
		            {
								uint8_t Byte4Return = GetSPIByte(i+1);
								if (((Byte4Return & BIT7HI) != BIT7HI) && ((Byte4Return & BIT6HI) == BIT6HI))
								{
										WholeGameStatus[(i-1)*3+i-1] = BLUE;
									//printf("Got Blue \r\n");

								}
								else if (((Byte4Return & BIT7HI) == BIT7HI) && ((Byte4Return & BIT6HI) != BIT6HI))
								{
										WholeGameStatus[(i-1)*3+i-1] = RED;
									//printf("Got Red \r\n");
								}
								else if (((Byte4Return & BIT7HI) != BIT7HI) && ((Byte4Return & BIT6HI) != BIT6HI))
								{
										WholeGameStatus[(i-1)*3+i-1] = STATION_UNCLAIMED;
									//printf("Got Unclaimed \r\n");
								} 							
								
								
								if (((Byte4Return & BIT5HI) != BIT5HI) && ((Byte4Return & BIT4HI) == BIT4HI))
								{
										WholeGameStatus[(i-1)*3+i] = BLUE;
									//printf("Got Blue \r\n");
								}
								else if (((Byte4Return & BIT5HI) == BIT5HI) && ((Byte4Return & BIT4HI) != BIT4HI))
								{
										WholeGameStatus[(i-1)*3+i] = RED;
									//printf("Got Red \r\n");
								}
								else if (((Byte4Return & BIT5HI) != BIT5HI) && ((Byte4Return & BIT4HI) != BIT4HI))
								{
										WholeGameStatus[(i-1)*3+i] = STATION_UNCLAIMED;
									//printf("Got Unclaimed \r\n");
								}
								
								if (((Byte4Return & BIT3HI) != BIT3HI) && ((Byte4Return & BIT2HI) == BIT2HI))
								{
										WholeGameStatus[(i-1)*3+i+1] = BLUE;
									//printf("Got Blue \r\n");
								}
								else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI))
								{
										WholeGameStatus[(i-1)*3+i+1] = RED;
								//	printf("Got Red \r\n");
								}
								else if (((Byte4Return & BIT3HI) != BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI))
								{
										WholeGameStatus[(i-1)*3+i+1] = STATION_UNCLAIMED;
									//printf("Got Unclaimed \r\n");
								}

								
								if (((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))
								{
										WholeGameStatus[(i-1)*3+i+2] = BLUE;
									//printf("Got Blue \r\n");
								}
								else if (((Byte4Return & BIT1HI) == BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))
								{
										WholeGameStatus[(i-1)*3+i+2] = RED;
									//printf("Got Red \r\n");
								}
								else if (((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))
								{
										WholeGameStatus[(i-1)*3+i+2] = STATION_UNCLAIMED;
									//printf("Got Unclaimed \r\n");
								}

							}							
							ES_Event Event7Post;				  
							Event7Post.EventType = WHOLE_GAME_STATUS_COLLECTED;
							PostMasterSM (Event7Post);
							  
			//				printf("\n\r WHOLE_GAME_STATUS_COLLECTED SENT \n\r");
						}							
      break;

      case SENDQUERYCOMMAND:
           CurrentEvent = DuringSendCommand(CurrentEvent);
			      
			      if (ES_TIMEOUT == CurrentEvent.EventType && BYTETIMEOUT_TIMER == CurrentEvent.EventParam )
						{ 
                //printf("\n\r IN SENDQUERYCOMMAND PAC\n\r");	
 							  NextState = WAITING4COMMAND;
					      MakeTransition = true; 
							  uint8_t Byte4Return = GetSPIByte(3);
							  //printf("\n\r Byte4Return %d\n\r", Byte4Return);	
								ES_Event Event6Post;
							  
							 if (((Byte4Return & BIT7HI) != BIT7HI) && ((Byte4Return & BIT6HI) != BIT6HI))
							 {
								 QueryStatusByte[0] =  NACK; // 0,0 = NACK
								 //printf("\n\r QueryStatusByte[0] =  NACK;\n\r");	
							 }
							 
							 else if (((Byte4Return & BIT7HI) != BIT7HI) && ((Byte4Return & BIT6HI) == BIT6HI))
							 {
								  
								 QueryStatusByte[0] =  ACK; // 0,1 = ACK
							 }
							 
							 else if (((Byte4Return & BIT7HI) == BIT7HI) && ((Byte4Return & BIT6HI) != BIT6HI))
							 {
                  QueryStatusByte[0] = BLOCKED; // 1,0 = blocked
							 }								 
						    
							 else if (((Byte4Return & BIT7HI) == BIT7HI) && ((Byte4Return & BIT6HI) == BIT6HI))
							 {
								  QueryStatusByte[0] =  BUSY; // 1,1 = busy
							 } else {
								 // has to put an else here to let the code run
							 }

							if (((Byte4Return & BIT5HI) == BIT5HI) && ((Byte4Return & BIT4HI) == BIT4HI))
							 {
								   QueryStatusByte[1] = COLOR_UNDEFINED; // 1,1 = undifined, not used
							 }			

							 else if (((Byte4Return & BIT5HI) != BIT5HI) && ((Byte4Return & BIT4HI) == BIT4HI))
							 {
								   QueryStatusByte[1] = BLUE; // 0,1 = BLUE 
							 }		
							 
							 else if (((Byte4Return & BIT5HI) == BIT5HI) && ((Byte4Return & BIT4HI) != BIT4HI))
							 {
								  QueryStatusByte[1] = RED; // 1,0 = RED
							 }		
							 
							 else if (((Byte4Return & BIT5HI) != BIT5HI) && ((Byte4Return & BIT4HI) != BIT4HI))
							 {
								  QueryStatusByte[1] = COLOR_NOTCLAIMED; // 0,0 = unclaimed
							 } else {

							 }		

               if (((Byte4Return & BIT3HI) != BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))
							 {
                 // 0001 Sacramento
								 QueryStatusByte[2] = SACRAMENTO;
					//			 printf("\r\n Sacramento captured");
							 }	
               else if (((Byte4Return & BIT3HI) != BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) == BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))
               {
								 // 0010 Seattle
								 QueryStatusByte[2] = SEATTLE;
					//			 printf("\r\n Seattle captured");
               }	
				       else if (((Byte4Return & BIT3HI) != BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) == BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))			 
							 {
								 
								// 0011 Billings
								 QueryStatusByte[2] = BILLINGS;
						//		 printf("\r\n Billings captured");
							 }
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))			 
							 {
								 
								// 1000 Denver
								 QueryStatusByte[2] = DENVER;
						//		 printf("\r\n Denver captured");
							 }		
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))			 
							 {			 
								// 1001 Dallas
								 QueryStatusByte[2] = DALLAS;
							//	 printf("\r\n Dallas captured");
							 }		
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) == BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))			 
							 {
								 
								// 1010 Chicago
								 QueryStatusByte[2] = CHICAGO;
						//		 printf("\r\n Chicago captured");
							 }					
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) != BIT2HI) && ((Byte4Return & BIT1HI) == BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))			 
							 {
			 
								// 1011 Miami
								 QueryStatusByte[2] = MIAMI;
						//		 printf("\r\n Miami captured");
							 }		
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) == BIT2HI) && ((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) != BIT0HI))			 
							 {
								 
								// 1100 Washington, DC
								 QueryStatusByte[2] = WASHINGTONDC;
					//			 printf("\r\n DC captured");
							 }		
				       else if (((Byte4Return & BIT3HI) == BIT3HI) && ((Byte4Return & BIT2HI) == BIT2HI) && ((Byte4Return & BIT1HI) != BIT1HI) && ((Byte4Return & BIT0HI) == BIT0HI))			 
							 {						 
								// 1101 Concord
								 QueryStatusByte[2] = CONCORD;
						//		 printf("\r\n Concord captured");
							 }	else {
                // let the code run
							 }								 
							 uint8_t Byte5Return = GetSPIByte(2);
							  if (Byte5Return == 0x00)
								{
									ResponseReadByte[0] = RESPONSE_NOT_READY;
								}
								else if(Byte5Return == 0xAA)
								{
								  ResponseReadByte[0] = RESPONSE_READY;
								} else
								{
									// has to put an else here to let the code run
								}
							 // location to be put here later
								//QueryStatusByte[2] = 0;
								Event6Post.EventType = QUERY_COLLECTED;
								PostMasterSM(Event6Post);
								ReturnEvent.EventType = ES_NO_EVENT;
						}		
//						  printf("QueryStatusByte 0: %x\r\n", QueryStatusByte[0]);
//						  printf("QueryStatusByte 1: %x\r\n", QueryStatusByte[1]);
//						  printf("QueryStatusByte 2: %x\r\n", QueryStatusByte[2]);
//						  printf("ResponseReadByte 0: %x\r\n", ResponseReadByte[0]);
						 // ReturnEvent.EventType = ES_NO_EVENT;
      break;			     
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunPACSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunPACSM(EntryEventKind);
     }
     return(ReturnEvent);
}

void StartPACSM ( ES_Event CurrentEvent)
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
		    //printf("\n\r StartPACSM \r\n");
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunPACSM(CurrentEvent);
	 //printf("\r\nStartSRunPACSM.");
}


PACState_t QueryPACSM ( void )
{
   return(CurrentState);
}

uint8_t GetWholeGameStatus(uint8_t i)
{
	return WholeGameStatus[i];
}

uint8_t GetQueryStatus(uint8_t i)
{
	return QueryStatusByte[i];
}

uint8_t GetResponseReady(uint8_t i)
{
	return ResponseReadByte[i];
}

uint8_t GetFrequency(void)
{
	return Freq;
}
/***************************************************************************
 private functions
 ***************************************************************************/



static ES_Event DuringWaiting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
       
    } else if ( Event.EventType == ES_EXIT )  {
      
    } else {
    // do the 'during' function for this state
      // in this case, do nothing and return the event up to the run function
    }
    return(ReturnEvent);
}

static ES_Event DuringSendCommand( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
       StartSendByte(Event);
    } else if ( Event.EventType == ES_EXIT )  {
       RunSendByte(Event);
    } else {
       ReturnEvent = RunSendByte(Event);
    }
    return(ReturnEvent);
}







