/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef PACSM_H
#define PACSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { WAITING4COMMAND, SENDOCCUPYCOMMAND, SENDCHECKSTATUS, SENDQUERYCOMMAND } PACState_t ;


// Public Function Prototypes

ES_Event RunPACSM( ES_Event CurrentEvent );
void StartPACSM ( ES_Event CurrentEvent);
PACState_t QueryPACSM ( void );
uint8_t GetQueryStatus(uint8_t); //0 - 2 
uint8_t GetResponseReady(uint8_t); //0
uint8_t GetWholeGameStatus(uint8_t); // 0 - 11
uint8_t GetFrequency(void);

#endif /*PACSM_H */

