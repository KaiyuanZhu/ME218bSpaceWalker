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
#include "PACSM.h"
#include "TOPPAC.h"
#include "GamePlaySM.h"

ES_Event RunTopPAC( ES_Event CurrentEvent );
void StartTopPAC ( ES_Event CurrentEvent );

#define STATUS_COMMAND 0x00C0
#define CHANGESTATION_COMMAND 0x0080 // This will change 0b10MR IIII, MY COLOR: 0 = RED, 1=BLUE, R Requested Color, IIII Frequency
#define QUERY_COMMAND1 0x0070
#define ENTRY_STATE WAIT4COMMAND

uint8_t GetQueryResult(uint8_t); //0 - 2 
uint8_t GetResponseReadyResult(uint8_t); //0
uint8_t GetGameStatus(uint8_t); // 0 - 11



static ES_Event DuringWait4command (ES_Event Event);
static ES_Event DuringOccupying( ES_Event Event);
static ES_Event DuringGettingStatus( ES_Event Event);
static ES_Event DuringSendQuery( ES_Event Event);

static TopPACState_t CurrentState = WAIT4COMMAND;
static uint8_t Occupy_Counter = 0;
static uint8_t First_Frequency_Flag = 0; // set it to 1 when the first frequency is ACK
static uint8_t Second_Frequency_Flag = 0; // set it to 1 when the second frequency is ACK
//static uint8_t Frequency_Saved = 0; // Save the first frequency
static uint8_t LEDColor;
static uint8_t Query_Counter = 0;
//static uint8_t City ;

ES_Event RunTopPAC( ES_Event CurrentEvent )
{
    bool MakeTransition = false;/* are we making a state transition? */
    TopPACState_t NextState = CurrentState;
    ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
    ES_Event Event2Post;
		//printf("\r\n in TOPPAC RUN \r\n");
    switch (CurrentState)
    {
        case WAIT4COMMAND :
					//printf("\r\n in TOPPAC WAIT4COMMAND \r\n");
        CurrentEvent = DuringWait4command(CurrentEvent);
        if ( CurrentEvent.EventType != ES_NO_EVENT )
        {   
					//printf("\r\n IF IS NOT ES_NOEVENT\r\n");
					switch (CurrentEvent.EventType)
					{
						case CHANGING_STATIONSTATUS_TOPPAC: 
						    NextState = CHANGING_STATION_STATUS;
                MakeTransition = true;
						//    printf("\r\n changing station request received \r\n");
                // uint8_t NewFrequency = CurrentEvent.EventParam;
						    uint8_t NewFrequency = CurrentEvent.EventParam;
//						    Frequency_Saved = NewFrequency; // Save this, when the response is BUSY, we can keep posting it.
                Event2Post.EventType = OCCUPYCOMMAND_PACSM;
							 LEDColor = GetLEDColor();
	             //LEDColor = 0x30;
	            //  printf("\r\n CurrentLEDColor is: %x",LEDColor);
                // add color here too
                Event2Post.EventParam = (CHANGESTATION_COMMAND + LEDColor + NewFrequency);
						    //printf("\n\r LED Command is: %x\n\r",LEDColor);
						  //  printf("\n\r 1st Command is: %x\n\r",(CHANGESTATION_COMMAND + LEDColor + NewFrequency));
                PostMasterSM (Event2Post);
						    Occupy_Counter++;
						break;
						  
						case GETTING_GAME_STATUS_TOPPAC:
                NextState = GETTING_STATION_STATUS;
                MakeTransition = true;			
                Event2Post.EventType = STATUSCOMMAND_PACSM;
                Event2Post.EventParam = STATUS_COMMAND;
                PostMasterSM (Event2Post);		
						 //   printf("STATUS COMMAND SENT");
						break;
						
						case QUERYING_TOPPAC:
							  NextState = SEND_QUERY;
                MakeTransition = true;
                Event2Post.EventType = QUERYCOMMAND_PACSM;
                Event2Post.EventParam = QUERY_COMMAND1;
						    Query_Counter ++;	
                PostMasterSM (Event2Post);
						 //   printf("QUERY COMMAND SENT");
						break;
						

					}  
        }
        break;
        
        case CHANGING_STATION_STATUS:
        CurrentEvent = DuringOccupying(CurrentEvent);
        if (CurrentEvent.EventType != ES_NO_EVENT)
        {
            switch (CurrentEvent.EventType)
            {
							  case OCCUPY_SENT_PACSM:
									//printf("\n\r OCCUPY COMMAND SENT \n\r");
									NextState = CHANGING_STATION_STATUS;
                  MakeTransition = true;
								  ES_Timer_InitTimer(QUERY_TIMER, 20);   
								 ReturnEvent.EventType = ES_NO_EVENT;
							  break;
								
								case ES_TIMEOUT:
									if (CurrentEvent.EventParam == QUERY_TIMER) // wait is over, query
									{ 
										//printf("\n\r QUERY_TIMER TIMEOUT \n\r");
									  Event2Post.EventType = QUERYCOMMAND_PACSM; // after sending out the first frequency, query
										Event2Post.EventParam = QUERY_COMMAND1;
									  PostMasterSM (Event2Post); 
										Query_Counter ++;	
										ReturnEvent.EventType = ES_NO_EVENT;
										MakeTransition = true; 
										NextState = CHANGING_STATION_STATUS;
									}
									
								break;
								
							
                case OCCUPY_SUCCEED: 
                NextState = WAIT4COMMAND;
                MakeTransition = true;
                First_Frequency_Flag = 0; // Reset the flags when succeed
                Second_Frequency_Flag = 0;	
                Occupy_Counter = 0;	
                Query_Counter = 0;									
								//Frequency_Saved = 0;
                // Upper level will get the event GAME_STATUS_COLLECTED, then user can directly call the function GetGameStatus to 	do some comparing and decide furture move		
                ReturnEvent.EventType = ES_NO_EVENT; // Consume the OCCUPY_SUCCEED
    						Event2Post.EventType = OCCUPY_IS_SUCCEED;								
						//		printf("/n/r Sending OCCUPY_IS_SUCCEED /n/r");
								uint8_t City = GetQueryStatus(2);
								Event2Post.EventParam = City;
						//		printf("\r\n in TOPPAC, City is %d", City);
								if (LEDColor == 0x00 && GetQueryResult(1) == 10){
									PostMasterSM (Event2Post);	//Red
								} else{
									PostMasterSM (Event2Post);	//Blue, should use else if here, but shouldn't matter
								}
								
                break;
								
								case SEND_SECOND_FREQ:
								NextState = CHANGING_STATION_STATUS;
								if ( First_Frequency_Flag == 1 && Second_Frequency_Flag == 0) // First Frequency is valid
								{
								  // Measure the second frequency at the polling station
							   	// Function needed here, GetFrequency()
							//		printf("/n/r Sending Second Frequency /n/r");
									uint8_t frequency = CurrentEvent.EventParam;
//									Frequency_Saved = frequency;
								  Event2Post.EventType = OCCUPYCOMMAND_PACSM;
                  Event2Post.EventParam = ( CHANGESTATION_COMMAND + LEDColor + frequency);
								//	printf("/n/r New Command is: %x/n/r",( CHANGESTATION_COMMAND + LEDColor + frequency));
                  PostMasterSM (Event2Post);
									Occupy_Counter++; // Now this should be 2													
//                  Event2Post.EventType = QUERYCOMMAND_PACSM; // after sending the second frequency, keep querying
//                  Event2Post.EventParam = QUERY_COMMAND1;
//									PostMasterSM (Event2Post);		
                  ES_Timer_InitTimer(QUERY_TIMER, 20); 									
								
								} 
								else if (First_Frequency_Flag == 0 && Second_Frequency_Flag == 0) // Waiting for Query Response
								{
                  Event2Post.EventType = QUERYCOMMAND_PACSM;
                  Event2Post.EventParam = QUERY_COMMAND1;
									PostMasterSM (Event2Post);
									Query_Counter ++;
							//		printf("\n\r Try to send second frequency, but both flag is 0, keep querying \n\r");
								} 
//								else 
//								{
//									
//								}
                MakeTransition = true;
								ReturnEvent.EventType = ES_NO_EVENT;		// Consume the SEND_SECOND_FREQ			
                break;
														
								
								case OCCUPY_BUSY: // Keep Querying with the same frequency
                NextState =  CHANGING_STATION_STATUS;
                MakeTransition = true;
								Event2Post.EventType = QUERYCOMMAND_PACSM;
                // add color here too
                Event2Post.EventParam = QUERY_COMMAND1;
								Query_Counter ++;
                PostMasterSM (Event2Post);							
								ReturnEvent.EventType = ES_NO_EVENT;
                break;
								
								
								case FREQUENCY_GOT:
											NextState = CHANGING_STATION_STATUS;
								      MakeTransition = true;
											Event2Post.EventType = SEND_SECOND_FREQ;
								      Event2Post.EventParam = CurrentEvent.EventParam;
							//	      printf("\n\r Second Frequency Sent  %x\n\r", CurrentEvent.EventParam);
								      PostMasterSM (Event2Post);
								      ReturnEvent.EventType = ES_NO_EVENT;
								      //printf("\n\r In Wait for second frequency \n\r");
  							break;
																
								case QUERY_COLLECTED:
								   
								  // printf("\n\r in QUERY_COLLECTED case \n\r");
								  if (GetResponseReady(0) == 1 && First_Frequency_Flag == 0 && Second_Frequency_Flag == 0) // Response is ready, and currently is dealing with the first frequency response
									{
										if (GetQueryStatus(0) == 1 ) // ACK, valid frequency
										{
											NextState = CHANGING_STATION_STATUS;
											Event2Post.EventType = WAIT_FOR_FREQUENCY;
									    PostMasterSM (Event2Post);
											Query_Counter = 0;
											First_Frequency_Flag = 1;
											ReturnEvent.EventType = ES_NO_EVENT;
											MakeTransition = true;
										//	printf("\n\r ACK, 1st command \n\r");
										}
										else if (GetQueryStatus(0) == 0) // NACK, invalid frequency
										{
											NextState = WAIT4COMMAND;
											Event2Post.EventType = WRONG_FIRST_FREQUENCY;
											Occupy_Counter = 0; // Reset the Occupy Counter
//											Frequency_Saved = 0; // Reset the saved frequency
									    PostMasterSM (Event2Post);
											ReturnEvent.EventType = ES_NO_EVENT;
											MakeTransition = true;
											//City = 0;
								//			printf("\n\r NACK, 1st command \n\r");
										} 
										else if (GetQueryStatus(0) == 10) // BLOCKED
										{
											NextState = WAIT4COMMAND; // Post and event and go back to waiting for new command
											Event2Post.EventType = OCCUPY_BLOCKED;
											Occupy_Counter = 0; // Reset the Occupy Counter
//											Frequency_Saved = 0; // Reset the saved frequency
											PostMasterSM (Event2Post);
											ReturnEvent.EventType = ES_NO_EVENT;
											MakeTransition = true;
											//City = 0;
                 //     printf("\n\r BLOCKED, 1st command \n\r");											
										}
										else if (GetQueryStatus(0) == 11) //BUSY
										{
											NextState = CHANGING_STATION_STATUS;
											Query_Counter = 0;
											Event2Post.EventType = OCCUPY_BUSY; // Go to Busy State and Keep Querying
									    PostMasterSM (Event2Post);
											ReturnEvent.EventType = ES_NO_EVENT;	
											MakeTransition = true;
											//City = 0;
								//			printf("\n\r BUSY, 1st command \n\r");
										}
									} 
									else if (GetResponseReady(0) == 1 && First_Frequency_Flag == 1 && Second_Frequency_Flag == 0) // Response is ready, and currently is dealing with second frequency
									{
											if (GetQueryStatus(0) == 1 ) // ACK, valid frequency
										{
											NextState = CHANGING_STATION_STATUS;
											Event2Post.EventType = OCCUPY_SUCCEED;
											//City = GetQueryStatus(2);
							
									    PostMasterSM (Event2Post);
											Second_Frequency_Flag = 1;
											Query_Counter = 0;
											ReturnEvent.EventType = ES_NO_EVENT;
											MakeTransition = true;
									//		printf("\n\r ACK, 2nd command \n\r");
										}					
                    else if(GetQueryStatus(0) == 0) // NACK, invald frequency
										{
											NextState = WAIT4COMMAND;
											Event2Post.EventType = WRONG_SECOND_FREQUENCY;
									    PostMasterSM (Event2Post);
											Occupy_Counter = 0; // Reset the Occupy Counter
//											Frequency_Saved = 0; // Reset the saved frequency	
                      First_Frequency_Flag = 0;											
											ReturnEvent.EventType = ES_NO_EVENT;	
											MakeTransition = true;
											//City = 0;
                //      printf("\n\r NACK, 2nd Command \n\r");											
										}											
										else if (GetQueryStatus(0) == 10) // BLOCKED
										{
											NextState = WAIT4COMMAND; // Post and event and go back to waiting for nwe command
											Event2Post.EventType = OCCUPY_BLOCKED;
											PostMasterSM (Event2Post);
											Occupy_Counter = 0; // Reset the Occupy Counter
//											Frequency_Saved = 0; // Reset the saved frequency			
                      First_Frequency_Flag = 0;											
											ReturnEvent.EventType = ES_NO_EVENT;	
                      MakeTransition = true;							
											//City = 0;
                //      printf("\n\r BLOCKED, 2nd command \n\r");											
										}
										else if (GetQueryStatus(0) == 11) // BUSY
										{
											NextState = CHANGING_STATION_STATUS;
											Event2Post.EventType = OCCUPY_BUSY; // Go to Busy State and Keep Querying
									    PostMasterSM (Event2Post);
											Query_Counter = 0;
											ReturnEvent.EventType = ES_NO_EVENT;
                      MakeTransition = true;
                      //City = 0;											
            //          printf("\n\r BUSY, 2nd command \n\r");											
										}
									}
									else if (GetResponseReady(0) == 0) // Super PAC is not Ready For response, keep querying
									{
										  Query_Counter ++;						  
										  if (Query_Counter <= 50){
												MakeTransition = true;
												NextState = CHANGING_STATION_STATUS;
												ES_Timer_InitTimer(QUERY_TIMER, 20); 
											} else {
												MakeTransition = true;
												NextState = WAIT4COMMAND;	
												Event2Post.EventType = WRONG_SECOND_FREQUENCY;
									      PostMasterSM (Event2Post);
                        Query_Counter = 0;
                        First_Frequency_Flag = 0; // Reset the flags when quit
                        Second_Frequency_Flag = 0;	
                        Occupy_Counter = 0;													
											}									    						
								      ReturnEvent.EventType = ES_NO_EVENT;
								//		  printf("\n\r Response Not Ready, set query timer again \n\r");
									}
								  
								break;
            }
        }
        break;
        
        case GETTING_STATION_STATUS:
        CurrentEvent = DuringGettingStatus(CurrentEvent);
        if (CurrentEvent.EventType != ES_NO_EVENT)
        {
						 printf("\r\n in GETTING_GAME_STATUS_TOPPAC \r\n");
            switch (CurrentEvent.EventType)
            {
                case WHOLE_GAME_STATUS_COLLECTED: // post by lower level
                NextState = WAIT4COMMAND;
                MakeTransition = true;
                printf("Whole Station Status 1: %x\r\n", GetWholeGameStatus(0));
                printf("Whole Station Status 2: %x\r\n", GetWholeGameStatus(1));
                printf("Whole Station Status 3: %x\r\n", GetWholeGameStatus(2));
                printf("Whole Station Status 4: %x\r\n", GetWholeGameStatus(3));
                printf("Whole Station Status 5: %x\r\n", GetWholeGameStatus(4));
                printf("Whole Station Status 6: %x\r\n", GetWholeGameStatus(5));
                printf("Whole Station Status 7: %x\r\n", GetWholeGameStatus(6));
                printf("Whole Station Status 8: %x\r\n", GetWholeGameStatus(7));
                printf("Whole Station Status 9: %x\r\n", GetWholeGameStatus(8));
                printf("Whole Station Status RED ATTACK: %x\r\n", GetWholeGameStatus(9));
                printf("Whole Station Status BLUE ATTACK: %x\r\n", GetWholeGameStatus(10));
                printf("Whole Station Status GAME STATUS: %x\r\n", GetWholeGameStatus(11));
								ES_Event Event2Post;
								Event2Post.EventType = GAME_STATUS_COLLECTED;
								PostMasterSM (Event2Post);
								ReturnEvent.EventType = ES_NO_EVENT; // consume the WHOLE_GAME_STATUS_COLLECTED
                break;
                
//                case WHOLE_GAME_STATUS_NOT_COLLECTED: // to be post by upper level
//                NextState = WAIT4COMMAND;
//                MakeTransition = true;
//								ReturnEvent.EventType = ES_NO_EVENT;
//                break;
            }
        }
        break;
        
        case SEND_QUERY:
        CurrentEvent = DuringSendQuery(CurrentEvent);
				//printf("\n\r IN SEND_QUERY\n\r");	
        if ( CurrentEvent.EventType != ES_NO_EVENT )
        {
            switch(CurrentEvent.EventType)
            {
                case QUERY_COLLECTED:
                NextState = WAIT4COMMAND;
                MakeTransition = true;
//                printf("Query Status Acknowledge: %d\r\n", GetQueryStatus(0));
//                printf("Query Status Color: %d\r\n", GetQueryStatus(1));
//                printf("Query Status Location: %d\r\n", GetQueryStatus(2));
//                printf("ResponseReady Status: %d\r\n", GetResponseReady(0));
								NextState = WAIT4COMMAND;
                MakeTransition = true;
								ReturnEvent.EventType = ES_NO_EVENT; // Consume the QUERY_COLLECTED
								Event2Post.EventType = QUERY_IS_COLLECTED;
								PostMasterSM (Event2Post);
                break;
								
//								case QUERY_NOT_COLLECTED:
//                NextState = WAIT4COMMAND;
//                MakeTransition = true;									
//								break;
            }
        }
        break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunTopPAC(CurrentEvent);
        
        CurrentState = NextState; //Modify state variable
        
        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunTopPAC(EntryEventKind);
    }
    return(ReturnEvent);
}



void StartTopPAC ( ES_Event CurrentEvent)
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
	 SPIInit();

   // call the entry function (if any) for the ENTRY_STATE
   RunTopPAC(CurrentEvent);
}

uint8_t GetGameStatus(uint8_t i)
{
	return GetWholeGameStatus(i);
}

uint8_t GetQueryResult(uint8_t i)
{
	return GetQueryStatus(i);
}

uint8_t GetResponseReadyResult(uint8_t i)
{
	return GetResponseReady(i);
}

// Private Functions
static ES_Event DuringWait4command (ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
    } else if ( Event.EventType == ES_EXIT ) {
    } else {
    }
    return(ReturnEvent);
}
static ES_Event DuringOccupying( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        StartPACSM( Event);
    } else if ( Event.EventType == ES_EXIT ) {
        RunPACSM( Event);
    } else {
        ReturnEvent = RunPACSM(Event);
    }
    return(ReturnEvent);
}

static ES_Event DuringGettingStatus( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        StartPACSM( Event);
			ES_Event Event2Post;
			Event2Post.EventType = STATUSCOMMAND_PACSM;
      Event2Post.EventParam = STATUS_COMMAND;
      PostMasterSM(Event2Post);

    } else if ( Event.EventType == ES_EXIT ) {
        RunPACSM( Event);
    } else {
        ReturnEvent = RunPACSM(Event);
    }
    return(ReturnEvent);
}

static ES_Event DuringSendQuery( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) ) {
        StartPACSM( Event);
    } else if ( Event.EventType == ES_EXIT ) {
        RunPACSM( Event);
    } else {
        ReturnEvent = RunPACSM(Event);
    }
    return(ReturnEvent);
}
