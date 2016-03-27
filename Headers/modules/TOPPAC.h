

#ifndef TOPPAC_H
#define TOPPAC_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { WAIT4COMMAND, CHANGING_STATION_STATUS, GETTING_STATION_STATUS, SEND_QUERY } TopPACState_t ;


// Public Function Prototypes

ES_Event RunTopPAC( ES_Event CurrentEvent );
void StartTopPAC ( ES_Event CurrentEvent );
uint8_t GetQueryResult(uint8_t); 
//0 - 2, 
//0 [Acknowledge] {NACK = 0, ACK = 1, BLOCKED = 10, BUSY = 11} 
//1 [Red/Blue/None {RED = 10, BLUE = 1,COLOR_NOTCLAIMED = 0, COLOR_UNDEFINED = 11}, ] 
//2 [Location {SACRAMENTO = 1, SEATTLE =2, BILLINGS = 3, DENVER = 4, DALLAS = 5, CHICAGO = 6, MIAMI = 7, WASHINGTONDC = 8, CONCORD= 9}]
uint8_t GetResponseReadyResult(uint8_t); //0 [ready for response = 1, response not ready = 0]
uint8_t GetGameStatus(uint8_t); 
// 0 - 11 (0)station 1, (1)station 2, (2)station 3, (3)station 4, (4)station 5, (5)station 6, 
 // (6)station 7, (7)station 8, (8)station 9, (9)Attack AD Status RED, (10)Attack AD Status BLUE, (11)Game Status
// for each station  STATION_UNCLAIMED = 0 BLUE = 1 RED = 10
// RED_UNDERATTACK = 1, RED_NOT_UNDERATTACK = 0 // BLUE_UNDERATTACK = 1, BLUE_NOT_UNDERATTACK = 0
// Game Status: GAME_WAITING 0, GAME_CHAMPAIGING 1

// Instruction on how to use SPI State Chart
// Three types of events can be grabed by TOPPAC: 
// 1. CHANGING_STATIONSTATUS_TOPPAC, Event Parameter to be passed with this event type: MRIIII, M means my color, R means requested Color (0 = RED, 1 = BLUE, IIII is the frequency)
// 2. GETTING_GAME_STATUS_TOPPAC
// 3. QUERYING_TOPPAC

// after posting the upper three events, the SPI State Chart will post events correspondingly to the MasterSM to indicates that the events has been processed and results are got.
//  For GETTING_GAME_STATUS_TOPPAC event, MasterSM will get an GAME_STATUS_COLLECTED event, then user can call function GetGameStatus(uint8_t) to access the data
//  For QUERYING_TOPPAC event, MasterSM will get an QUERY_IS_COLLECTED event, then user can call function GetQueryResult(uint8_t) and GetResponseReadyResult(uint8_t) to access the data
//  For CHANGING_STATIONSTATUS_TOPPAC event, TOPPAC will conduct the two step occupying procedure, the user should have a public function GetFrequency() ready, so that TOPPAC can have the new frequency when needed.
// When the OCCUPY IS SUCCESSFUL, MasterSM will get an OCCUPY_IS_SUCCEED event.
// When occupy is not successful, MasterSM will get any events among 	WRONG_FIRST_FREQUENCY, WRONG_SECOND_FREQUENCY, OCCUPY_BLOCKED,

// Also, when SPI gets and in-valid response from SuperPAC, it will post an event called INVALID_RESPONSE_RECEIVED. User should decide what to do with this information.




#endif /*TOPPAC_H */
